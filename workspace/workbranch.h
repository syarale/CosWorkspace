#ifndef WORKSPACE_WORKBRANCH_H_
#define WORKSPACE_WORKBRANCH_H_

#include <assert.h>
#include <condition_variable>
#include <future>
#include <mutex>
#include <unordered_map>
#include <functional>

#include "base/autothread.h"
#include "base/thread_safe_queue.h"
#include "base/utility.h"

namespace cos {
namespace workspace {

using cos::base::AutoThread;
using cos::base::ThreadSafeQueue;

using id = std::thread::id;
using Task = std::function<void()>;
using worker = AutoThread<cos::base::detach>;

namespace base = cos::base;

class WorkBranch {
 public:
  WorkBranch(int num = 1) {
    for (int i = 0; i < num; i ++) {
      AddWorker();
    }
  }

  WorkBranch(const WorkBranch& ) = delete;
  WorkBranch& operator= (WorkBranch&) = delete;
  WorkBranch(WorkBranch&&) = delete;
  
  ~WorkBranch() {
    std::unique_lock<std::mutex> ulk(mtx_);
    is_destructing_ = true;
    declines_ = workers_map_.size();
    destructing_cv_.wait(ulk, [this] { return declines_ <= 0;});
  }

  void AddWorker() {
    std::lock_guard<std::mutex> lock(mtx_);
    std::thread thrd(&WorkBranch::process, this);
    workers_map_.emplace(thrd.get_id(), std::move(thrd));
  }

  void RemoveWorker() {
    std::lock_guard<std::mutex> lock(mtx_);
    if (workers_map_.empty()) {
      std::cout << "[INFO] Invalid remove, wokers pool is empty." << std::endl;
    } else {
      declines_ ++;
    }
  }
  
  // Submit 'normal' task, and return void
  template <typename T = base::normal, typename F,
            typename R = base::result_of_t<F>,
            typename DR = typename std::enable_if<std::is_void<R>::value>::type>
  auto Submit(F&& task) -> typename std::enable_if<std::is_same<T, base::normal>::value>::type {
    tasks_que_.push_back([task] {
      task();
    });
  }
  
  // Submit 'urgent' task, and return void
  template <typename T, typename F,
            typename R = base::result_of_t<F>,
            typename DR = typename std::enable_if<std::is_void<R>::value>::type>
  auto Submit(F&& task) -> typename std::enable_if<std::is_same<T, base::urgent>::value>::type {
    tasks_que_.push_front([task] {
      task();
    });
  }

  // Submit 'normal' task, and return std::future<R>
  template <typename T = base::normal, typename F,
            typename R = base::result_of_t<F>,
            typename DR = typename std::enable_if<!std::is_void<R>::value>::type>
  auto Submit(F&& task) -> typename std::enable_if<std::is_same<T, base::normal>::value, std::future<R>>::type {
    std::function<R()> exec(std::forward<F>(task));
    std::shared_ptr<std::promise<R>> task_promise = std::make_shared<std::promise<R>>();
    tasks_que_.push_back([exec, task_promise] {
      task_promise->set_value(exec());
    });
    return task_promise->get_future();
  }

  // Submit 'urgent' task, and return std::future<R>
  template <typename T, typename F,
            typename R = base::result_of_t<F>,
            typename DR = typename std::enable_if<!std::is_void<R>::value>::type>
  auto Submit(F&& task) -> typename std::enable_if<std::is_same<T, base::urgent>::value, std::future<R>>::type {
    std::function<R()> exec(std::forward<F>(task));
    std::shared_ptr<std::promise<R>> task_promise = std::make_shared<std::promise<R>>();
    tasks_que_.push_front([exec, task_promise] {
      task_promise->set_value(exec());
    });
    return task_promise->get_future();
  }

  // Submit sequence of tasks, and return void.
  template <typename T, typename F, typename... Fs,
            typename R = base::result_of_t<F>,
            typename DR = typename std::enable_if<std::is_void<R>::value>::type>
  auto Submit(F&& task, Fs&&... tasks) -> typename std::enable_if<std::is_same<T, base::sequence>::value>::type {
    tasks_que_.push_back([=] {
      recursive_exec(task, tasks...);
    });
  }

  void WaitTasks() {
    std::unique_lock<std::mutex> ulk(mtx_);
    is_waiting_ = true;
    waiting_cv_.wait(ulk, [this] { return tasks_done_ >= workers_map_.size();});
    assert(tasks_done_ >= workers_map_.size());
    
    is_waiting_ = false;
    tasks_done_ = 0;
    recover_cv_.notify_all();
  }

  std::size_t WorkersNum() {
    std::lock_guard<std::mutex> lock(mtx_);
    return workers_map_.size();
  }

  std::size_t TasksNum() {
    std::lock_guard<std::mutex> lock(mtx_);
    return tasks_que_.size();
  }

 private:
  void process() {
    while(true) {
      Task task;
      if (declines_ > 0) {
        std::lock_guard<std::mutex> lock(mtx_);
        if (declines_ > 0) {    // double check
          declines_ --;
          workers_map_.erase(std::this_thread::get_id());
          if (is_destructing_) {
            destructing_cv_.notify_one();
          }

          if (is_waiting_) {
            waiting_cv_.notify_one();
          }
          return;
        }
      }

      if (is_waiting_) {
        std::unique_lock<std::mutex> ulk(mtx_);
        tasks_done_ ++;
        waiting_cv_.notify_one();
        recover_cv_.wait(ulk);
      }

      if (tasks_que_.try_pop(task)) {
        task();
      } else {
        std::this_thread::yield();
      }
    }
  }

  template <typename F>
  void recursive_exec(F&& task) {
    task();
  }
  
  template <typename F, typename... Fs>
  void recursive_exec(F&& task, Fs&&... tasks) {
    task();
    recursive_exec(std::forward<Fs>(tasks)...);
  }


  std::size_t tasks_done_ = 0;
  std::size_t declines_ = 0;                   // For Destructor and RemoveWorker
  bool is_destructing_ = false;
  bool is_waiting_ = false;

  std::condition_variable destructing_cv_;
  std::condition_variable waiting_cv_;
  std::condition_variable recover_cv_;
  std::mutex mtx_;

  std::unordered_map<id, worker> workers_map_;
  ThreadSafeQueue<Task> tasks_que_;
};


} // namespace workspace
} // namespace cos

#endif // WORKSPACE_WORKBRANCH_H_