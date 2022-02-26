cmake_minimum_required(VERSION 3.10.2)
project(TO)

set(CMAKE_CXX_STANDARD 14)

find_package(Threads)

add_executable(TO main.cpp src/TimeStamp.h src/TimeStamp.cc
        src/Engine.h src/Engine.cc
        src/Transaction.h src/Transaction.cc
        )

target_link_libraries(TO Threads::Threads)
