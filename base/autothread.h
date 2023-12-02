/*
 *  A thread-wrapper that automatically calls 
 *  the join()/detach() when the object is destructed.
 */

#ifndef base_AutoThread_H_
#define base_AutoThread_H_
#include <iostream>
#include <thread>

namespace cos {
namespace base {

// just for type inference
struct join {};
struct detach {};

template <typename T>
class AutoThread {

};

template <>
class AutoThread<join> {
 public:
  AutoThread(std::thread&& t) : thread_(std::move(t)) { }
  AutoThread(AutoThread&) = delete;
  AutoThread& operator= (const AutoThread&) = delete;
  AutoThread(AutoThread&& thrd) = default;

  ~AutoThread() {
    if (thread_.joinable()) {
      thread_.join();
    }
  }

 private:
  std::thread thread_;
};

template <>
class AutoThread<detach> {
 public:
  explicit AutoThread(std::thread&& t) : thread_(std::move(t)) { }
  AutoThread(AutoThread&) = delete;
  AutoThread& operator= (const AutoThread&) = delete;
  explicit AutoThread(AutoThread&& thrd) = default;

  ~AutoThread() {
    if (thread_.joinable()) {
      thread_.detach();
    }
  }

 private:
  std::thread thread_;
};

} // namespace base
} // namespace cos

#endif  // base_AutoThread_H_
