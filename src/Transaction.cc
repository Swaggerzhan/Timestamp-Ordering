//
// Created by swagger on 2022/2/25.
//

#include "Transaction.h"
#include "TimeStamp.h"
#include "Engine.h"

using std::string;

UndoRecord::UndoRecord(UndoType t, Record r)
: type(t)
, record(r)
{}



TransactionController::TransactionController()
: Tctx_(new TransactionContext)
{
  Tctx_->TS = -1;
}


bool TransactionController::BEGIN() {
  if ( Tctx_->TS == -1 ) {
    Tctx_->TS = TimeStamp::getTS();
    return true;
  }
  return false;
}

bool TransactionController::END() {
  bool ret = Engine::commit(Tctx_);
  clear();
  return ret;
}

void TransactionController::ABORT() {
  Engine::rollback(Tctx_);
  clear();
}


ExecState TransactionController::read(string& key, string& ret) {
  if ( Tctx_->TS == -1 ) {
    return TNotInit;
  }
  State state;
  ret = Engine::read(Tctx_, key, &state);
  if (state == Success) { // 获取成功，返回事务OK
    return TOK;
  }
  ABORT();
  return TFailed; // 事务错误
}

ExecState TransactionController::read(string&& key, string& ret) {
  if ( Tctx_->TS == -1 ) {
    return TNotInit;
  }
  State state;
  ret = Engine::read(Tctx_, key, &state);
  if (state == Success) { // 获取成功，返回事务OK
    return TOK;
  }
  ABORT();
  return TFailed; // 事务错误
}

ExecState TransactionController::write(string& key, string& val) {
  if ( Tctx_->TS == -1 ) {
    return TNotInit;
  }
  if (Engine::write(Tctx_, key, val, false) ) {
    return TOK;
  }
  ABORT();
  return TFailed;
}

ExecState TransactionController::write(string&& key, string& val) {
  if ( Tctx_->TS == -1 ) {
    return TNotInit;
  }
  if (Engine::write(Tctx_, key, val, false) ) {
    return TOK;
  }
  ABORT();
  return TFailed;
}


ExecState TransactionController::del(string& key) {
  if ( Tctx_->TS == -1 ) {
    return TNotInit;
  }
  string t;
  if ( Engine::write(Tctx_, key, t, true)) {
    return TOK;
  }
  ABORT();
  return TFailed;
}

void TransactionController::clear() {
  Tctx_->TS = -1;
  Tctx_->undoList.clear();
  Tctx_->commitQ.clear();
}













