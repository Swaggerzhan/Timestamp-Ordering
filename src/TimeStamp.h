//
// Created by swagger on 2022/2/25.
//

#ifndef TO_TIMESTAMP_H
#define TO_TIMESTAMP_H

#include <atomic>

class TimeStamp{
public:

  TimeStamp();

  ~TimeStamp()=default;

  static int64_t getTS();

private:
  static std::atomic<int64_t> t_;
};


#endif //TO_TIMESTAMP_H
