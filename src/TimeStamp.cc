//
// Created by swagger on 2022/2/25.
//

#include "TimeStamp.h"

TimeStamp::TimeStamp()
{}

std::atomic<int64_t> TimeStamp::t_{0};

int64_t TimeStamp::getTS() {
  return t_.fetch_add(1);
}

