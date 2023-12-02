cmake_minimum_required(VERSION 3.16)
project(CosWorkspace)

set(LIB_PATH ${PROJECT_SOURCE_DIR}/lib)
set(EXEC_PATH ${PROJECT_SOURCE_DIR}/bin)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${EXEC_PATH})

# For googletest
enable_testing()
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIR})

add_subdirectory(base)
add_subdirectory(workspace)