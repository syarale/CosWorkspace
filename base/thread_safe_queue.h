/*
 * A simple thread safe queue
 */

#ifndef base_THREAD_SAFE_QUEUE_
#define base_THREAD_SAFE_QUEUE_

#include <deque>
#include <mutex>

namespace cos {
namespace base {

template<typename T>
class ThreadSafeQueue {
 public:
  ThreadSafeQueue() {}
  ThreadSafeQueue(const ThreadSafeQueue&) = delete;
  ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;
  ~ThreadSafeQueue() { }

  void push_front(const T& element) {
    std::lock_guard<std::mutex> lock(mtx_);
    queue_.emplace_front(element);
  }

  void push_front(T&& element) {
    std::lock_guard<std::mutex> lock(mtx_);
    queue_.emplace_front(std::move(element));
  }

  void push_back(const T& element) {
    std::lock_guard<std::mutex> lock(mtx_);
    queue_.emplace_back(element);
  }

  void push_back(T&& element) {
    std::lock_guard<std::mutex> lock(mtx_);
    queue_.emplace_back(std::move(element));
  }

  bool try_pop(T& element) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (queue_.empty()) {
      return false;
    }
    element = std::move(queue_.front());
    queue_.pop_front();
    return true;
  }

  using size_type = typename std::deque<T>::size_type;
  size_type size() {
    std::lock_guard<std::mutex> lock(mtx_);
    return queue_.size();
  }

  bool empty() {
    std::lock_guard<std::mutex> lock(mtx_);
    return queue_.empty();
  }
  
 private:
  std::deque<T> queue_;
  std::mutex mtx_;
};


} // namespace base
} // namespace cos



#endif // base_THREAD_SAFE_QUEUE_