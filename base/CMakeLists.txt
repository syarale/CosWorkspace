cmake_minimum_required(VERSION 3.16)
project(base)

add_executable(thread_safe_queue_test thread_safe_queue_test.cpp)
target_link_libraries(thread_safe_queue_test pthread ${GTEST_BOTH_LIBRARIES})