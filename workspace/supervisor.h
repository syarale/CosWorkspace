#ifndef WORKSPACE_SUPERVISOR_H_
#define WORKSPACE_SUPERVISOR_H_

#include <assert.h>
#include <condition_variable>
#include <chrono>
#include <vector>
#include <mutex>

#include "workbranch.h"
#include "base/autothread.h"

namespace cos {
namespace workspace {

using CallbackFunc = std::function<void()>;
using cos::base::AutoThread;

constexpr std::size_t DEFAULT_INTERVALS = 500; // ms
constexpr std::size_t DEFAULT_MIN_WORKERS = 1;
constexpr std::size_t DEFAULT_MAX_WORKERS = 1;

class Supervisor { 
 public:
  explicit Supervisor(std::size_t min_workers, std::size_t max_workers,
                      std::size_t intervals = DEFAULT_INTERVALS) 
      : min_workers_num_(min_workers),
        max_workers_num_(max_workers),
        time_inervals_(intervals),
        kTimeInervals(intervals),
        worker_(std::thread(&Supervisor::Process, this)){
    // The validity of the input is guaranteed by the user
    assert(0 < min_workers_num_ && min_workers_num_ <= max_workers_num_);
  }

  Supervisor(const Supervisor&) = delete;
  Supervisor& operator=(const Supervisor&) = delete;

  ~Supervisor() {
    std::lock_guard<std::mutex> lock(mtx_);
    is_stop_ = true;
    tick_cv_.notify_one();
  }

  void Supervise(WorkBranch& branch) {
    std::lock_guard<std::mutex> lock(mtx_);
    branches_vec_.emplace_back(&branch);
  }

  void Suspend() {
    std::lock_guard<std::mutex> lock(mtx_);
    time_inervals_ = -1;
  }

  void Resume() {
    std::lock_guard<std::mutex> lock(mtx_);
    time_inervals_ = kTimeInervals;
    tick_cv_.notify_one();
  }

  void SetTickCallback(CallbackFunc func) {
    std::lock_guard<std::mutex> lock(mtx_);
    tick_callback_ = func;
  }

 private:
  void Process() {
    while (!is_stop_) {
      {
        std::unique_lock<std::mutex> ulk(mtx_);
        for (std::size_t i = 0; i < branches_vec_.size(); i ++) {
          WorkBranch* branch = branches_vec_[i];
          std::size_t works_num = branch->WorkersNum();
          std::size_t tasks_num = branch->TasksNum();
          if (tasks_num > 0) {
            if (works_num < max_workers_num_) {
              // TODO(leisy): Modify the policy of Add operation.
              for (std::size_t i = 0; i < max_workers_num_ - works_num; i ++) {
                branch->AddWorker();   // quikly - add
              }
            }
          } else {
            if (min_workers_num_ < works_num) {
              branch->RemoveWorker();  // slowly - remove
            }
          }
        }
       
        if (!is_stop_) {
          tick_cv_.wait_for(ulk, std::chrono::milliseconds(time_inervals_));
        }
      }
      tick_callback_();
    }
  }
  
  // It should be ensured that the WorkBranch is destroyed 
  // before the Supervisor
  std::vector<WorkBranch*> branches_vec_;

  CallbackFunc tick_callback_ = ([]{});
  bool is_stop_ = false;

  std::size_t min_workers_num_;
  std::size_t max_workers_num_;
  std::size_t time_inervals_;
  const std::size_t kTimeInervals;
  AutoThread<cos::base::join> worker_; 
  
  std::condition_variable tick_cv_;
  std::mutex mtx_;
};

}  // namespace workspace
}  // namespace cos

#endif  // WORKSPACE_SUPERVISOR_H_