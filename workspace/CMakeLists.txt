cmake_minimum_required(VERSION 3.16)
project(workspace)

add_executable(workbranch_test workbranch_test.cpp)
target_link_libraries(workbranch_test pthread ${GTEST_BOTH_LIBRARIES})

add_executable(supervisor_test supervisor_test.cpp)
target_link_libraries(supervisor_test pthread ${GTEST_BOTH_LIBRARIES})