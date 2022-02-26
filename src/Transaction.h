//
// Created by swagger on 2022/2/25.
//

#ifndef TO_TRANSACTION_H
#define TO_TRANSACTION_H

#include <atomic>
#include <map>
#include <memory>
#include <vector>
#include <set>

struct Entry;

typedef std::pair<std::string, std::shared_ptr<Entry>> Record;


// Undo型日志的操作类型
enum UndoType {
  Delete, // 删除
  Replace, // 覆盖
};

struct UndoRecord {
  UndoRecord(UndoType, Record);
  UndoType type;
  Record record;
};


struct TransactionContext {

  int64_t TS;
  std::vector<UndoRecord> undoList; // 撤销操作
  std::vector<std::shared_ptr<Entry>> commitQ;

};


enum ExecState {
  TOK,
  TFailed,
  TNotInit,
};

class TransactionController {
public:

  TransactionController();
  ~TransactionController()=default;
  bool BEGIN();
  void ABORT();
  bool END();

  ExecState read(std::string&, std::string&);
  ExecState read(std::string&&, std::string&);
  ExecState write(std::string&, std::string&);
  ExecState write(std::string&&, std::string&);
  ExecState del(std::string&);

private:

  void clear();

private:

  TransactionContext* Tctx_;

};







#endif //TO_TRANSACTION_H
