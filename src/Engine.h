//
// Created by swagger on 2022/2/25.
//

#ifndef TO_ENGINE_H
#define TO_ENGINE_H

#include <map>
#include <atomic>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>

struct TransactionContext;


struct Entry {

  Entry();

  std::mutex latch;
  std::condition_variable cv;

  int64_t W_TS;
  int64_t R_TS;
  bool C;
  std::string value;
  bool del;

};

enum State{
  Success,
  KeyNotExist,
  Failed,
};

typedef std::pair<std::string, std::shared_ptr<Entry>> Record;
typedef std::map<std::string, std::shared_ptr<Entry>> DataMap;

class Engine {
public:

  static std::string read(TransactionContext*, std::string, State*);

  static bool write(TransactionContext*, std::string&, std::string&, bool);
  static bool commit(TransactionContext*);

  static void rollback(TransactionContext*);


  // for debug
  static void show();
  static void insert(std::string&, std::string&);
  static void insert(std::string&&, std::string&&);

private:

  // 插入新值
  static void insert(TransactionContext*, std::string&, std::string&);

  // 更新旧值
  static void update(TransactionContext*, std::string&, std::string&, std::shared_ptr<Entry>&, bool);

  static bool getData(std::string&, std::shared_ptr<Entry>&);
  static bool writeData(std::string&, std::shared_ptr<Entry>);

private:

  static std::mutex mutex_;
  static DataMap data_;

};






#endif //TO_ENGINE_H
