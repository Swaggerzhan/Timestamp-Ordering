//
// Created by swagger on 2022/2/25.
//

#include "Engine.h"
#include "Transaction.h"

using std::string;


std::mutex Engine::mutex_{};
DataMap Engine::data_{};


static int64_t max(int64_t left, int64_t right) {
  if (left > right) {
    return left;
  }
  return right;
}

static void copyEntry(std::shared_ptr<Entry>&dst, std::shared_ptr<Entry>& src) {
  dst->W_TS = src->W_TS;
  dst->R_TS = src->R_TS;
  dst->value = src->value;
  dst->C = src->C;
  dst->del = src->del;
}



Entry::Entry()
: W_TS(0)
, R_TS(0)
, C(false)
, value()
, del(false)
{}

string Engine::read(TransactionContext* Tctx, std::string key, State* state) {
  std::shared_ptr<Entry> entry;
  if (!getData(key, entry)) {
    *state = KeyNotExist;
    return string();
  }

  std::unique_lock<std::mutex> lockGuard(entry->latch);

  // 读未来写，abort
  if ( Tctx->TS < entry->W_TS ) {
    *state = Failed;
    return string();
  }

  // 需要判断此时的数据是否已经提交了
  // 未提交，则abort或者等待
  while ( !entry->C ) {
    // 条件变量等待
    entry->cv.wait(lockGuard);
  }
  // 先更新
  entry->R_TS = max( Tctx->TS, entry->R_TS );
  string ret = entry->value;
  *state = Success;
  return ret;
}


bool Engine::write(TransactionContext* Tctx, string& key, string& val, bool isDel) {

  std::shared_ptr<Entry> entry;
  if ( !getData(key, entry)) { // 没有此数据
    // 插入新的值
    insert(Tctx, key, val);
    return true;
  }
  std::unique_lock<std::mutex> lockGuard(entry->latch);

  // 此字段被未来的某个事务读取过了，为了避免出错，只能放弃此次写入
  if ( Tctx->TS < entry->R_TS )  {
    return false;
  }

  // 考虑TWR的情况
  if ( Tctx->TS < entry->W_TS ) {
    while ( !entry->C ) {
      // 等待直到commit或者更高事务abort
      entry->cv.wait(lockGuard);
    }
    // 醒来后发现更高的事务被终止了
    if ( Tctx->TS >= entry->W_TS ) {
      // 正常写入
      update(Tctx, key, val, entry, isDel);
    }
    // 更高事务没有终止，TWR，直接忽略当前写入操作
    return true;
  }
  update(Tctx, key, val, entry, isDel);
  return true;
}


void Engine::insert(TransactionContext* Tctx, string& key, string& val) {
  std::shared_ptr<Entry> back(new Entry);
  Tctx->undoList.emplace_back(UndoRecord(Delete, Record(key, back)));
  back->W_TS = Tctx->TS;
  back->R_TS = Tctx->TS;
  back->C = false;
  back->del = false;
  back->value = val;
  writeData(key, back);

  Tctx->commitQ.push_back(back); // 提交队列
}


/*
 * @param Tctx: 事务上下文
 * @param key: 键
 * @param val: 新值
 * @param entry: 旧键值对指针
 * @param isDel: 是否删除
 */
void Engine::update(TransactionContext* Tctx, string& key, string& val, std::shared_ptr<Entry>& entry, bool isDel) {
  // 保存旧值
  std::shared_ptr<Entry> back(new Entry);
  copyEntry(back, entry); // TODO: 不需要锁
  Tctx->undoList.emplace_back(UndoRecord(Replace, Record(key, back)));
  // 写入
  entry->W_TS = max(entry->W_TS, Tctx->TS);
  entry->C = false;
  entry->value = val;
  entry->del = isDel;

  Tctx->commitQ.push_back(entry); // 提交队列
}


bool Engine::commit(TransactionContext* Tctx) {
  for ( auto iter: Tctx->commitQ) {
    iter->latch.lock();
    iter->C = true;
    iter->latch.unlock();
    iter->cv.notify_all();
  }
  return true;
}

void Engine::rollback(TransactionContext *Tctx) {

  while (!Tctx->undoList.empty()) {
    auto undoRecord = Tctx->undoList.back();
    Tctx->undoList.pop_back();
    std::shared_ptr<Entry> entry;
    getData(undoRecord.record.first, entry);
    if (undoRecord.type == Delete) { // 回滚执行删除操作
      entry->latch.lock();
      entry->del = true;
      entry->latch.unlock();
      entry->cv.notify_all();
    }else if ( undoRecord.type == Replace ) { // 回滚执行删除操作
      entry->latch.lock();

      entry->W_TS = undoRecord.record.second->W_TS;
      entry->R_TS = undoRecord.record.second->R_TS;
      entry->C = undoRecord.record.second->C;
      entry->value = undoRecord.record.second->value;
      entry->del = undoRecord.record.second->del;

      entry->latch.unlock();
      entry->cv.notify_all();
    }
  }
}


// for data

bool Engine::getData(std::string& key, std::shared_ptr<Entry> &entry) {
  std::lock_guard<std::mutex> lockGuard(mutex_);
  auto iter = data_.find(key);
  if ( iter == data_.end() ) {
    return false;
  }
  entry = iter->second;
  return true;
}


bool Engine::writeData(std::string& key, std::shared_ptr<Entry> entry) {
  Record record(key, entry);
  std::lock_guard<std::mutex> lockGuard(mutex_);
  data_.insert(record);
  return true;
}

// ************* for debug ************
#include <iostream>
using std::cout;
using std::endl;


// 删除的：(key->value)
// 未commit：[key->value]
void Engine::show() {
  cout << "------SHOW BEGIN------" << endl;
  std::unique_lock<std::mutex> lockGuard(mutex_);
  for (auto &iter : data_ ) {
    if (iter.second->del || !iter.second->C) {
      if (iter.second->del) {
        cout << "(";
        cout << iter.first << " -> ",
        cout << iter.second->value;
        cout << ")" << endl;
      }else if (!iter.second->C) {
        cout << "[";
        cout << iter.first << " -> ",
                cout << iter.second->value;
        cout << "]" << endl;
      }
    }else {
      cout << iter.first << " -> ",
      cout << iter.second->value << endl;
    }
  }
  cout << "------SHOW END--------" << endl;
}

void Engine::insert(string& key, string& val) {
  std::shared_ptr<Entry> entry(new Entry);
  entry->R_TS = 0;
  entry->W_TS = 0;
  entry->C = true;
  entry->del = false;
  entry->value = val;
  std::unique_lock<std::mutex> lockGuard(mutex_);
  data_.insert(Record(key, entry));
}

void Engine::insert(string&& key, string&& val) {
  std::shared_ptr<Entry> entry(new Entry);
  entry->R_TS = 0;
  entry->W_TS = 0;
  entry->C = true;
  entry->del = false;
  entry->value = val;
  std::unique_lock<std::mutex> lockGuard(mutex_);
  data_.insert(Record(key, entry));
}
