#ifndef WORKSPACE_WORKSPACE_H_
#define WORKSPACE_WORKSPACE_H_

#include <list>
#include <map>

#include "supervisor.h"
#include "workbranch.h"


namespace cos {
namespace workspace {

using cos::workspace::WorkBranch;
using cos::workspace::Supervisor;

using BranchList = std::list<std::unique_ptr<WorkBranch>>;
using SupervisorMap = std::map<const Supervisor*, std::unique_ptr<Supervisor>>;
using pos_t = BranchList::iterator;

class Bid {
 friend class WorkSpace;
 public:
  Bid(WorkBranch* branch) : branch_(branch) {}
  Bid(const Bid&) = default;
  Bid& operator=(const Bid&) = default;
  ~Bid() {}

  WorkBranch* branch() const {
    return branch_;
  }

  bool operator == (const Bid& other) {
    return branch_ == other.branch();
  }

  bool operator != (const Bid& other) {
    return branch_ != other.branch();
  }

  bool operator < (const Bid& other) {
    return branch_ < other.branch();
  }

  friend std::ostream& operator <<(std::ostream& os, const Bid& bid) {
    os << (uint64_t)(bid.branch());
    return os;
  }

 private:
  WorkBranch* branch_ = nullptr;
};


class Sid {
 friend class WorkSpace;
 public:
  Sid(Supervisor* super) : super_(super) {}
  Sid(const Sid&) = default;
  Sid& operator= (const Sid&) = default;
  ~Sid() {}

  Supervisor* super() const {
    return super_;
  }

  bool operator == (const Sid& other) {
    return super_ == other.super();
  }

  bool operator != (const Sid& other) {
    return super_ != other.super();
  }

  bool operator < (const Sid& other) {
    return super_ < other.super();
  }

  friend std::ostream& operator <<(std::ostream& os, const Sid& sid) {
    os << (uint64_t)(sid.super());
    return os;
  }

 private:
  Supervisor* super_;
};


class Workspace {
 public:
  explicit Workspace() {};
  
  ~Workspace() {
    branches_list_.clear();
    supers_map_.clear();
  }

  Workspace(const Workspace&) = delete;
  Workspace& operator=(const Workspace&) = delete;
  Workspace(Workspace&&) = delete;

  Bid Attach(WorkBranch* branch) {
    assert(branch != nullptr);
    branches_list_.emplace_back(branch);
    current_ = branches_list_.begin();
    return Bid(branch);
  }

  Sid Attach(Supervisor* super) {
    assert(super != nullptr);
    supers_map_.emplace(super, super);
    return Sid(super);
  }

  auto Detach(Bid bid) -> std::unique_ptr<WorkBranch> {
    for (auto it = branches_list_.begin(); it != branches_list_.end(); it ++) {
      if (it->get() == bid.branch()) {
        if (it == current_) {
          Forward(current_);
        }
        WorkBranch* branch = it->release();
        branches_list_.erase(it);
        if (branches_list_.empty()) {
          current_ = {};
        }
        return std::unique_ptr<WorkBranch>(branch);
      }
    }
    return nullptr;
  }

  auto Detach(Sid sid) -> std::unique_ptr<Supervisor> {
    auto it = supers_map_.find(sid.super());
    if (it != supers_map_.end()) {
      Supervisor* super = (it->second).release();
      supers_map_.erase(it);
      return std::unique_ptr<Supervisor>(super);
    }
    return nullptr;
  }

  void ForEach(std::function<void(WorkBranch&)> deal) {
    for (auto& branch_ptr : branches_list_) {
      deal(*(branch_ptr.get()));
    }
  }

  void ForEach(std::function<void(Supervisor&)> deal) {
    for (auto& supers : supers_map_) {
      deal(*((supers.second).get()));
    }
  }
 
  WorkBranch& operator[] (Bid bid) {
    return *(bid.branch());
  }

  Supervisor& operator[] (Sid sid) {
    return *(sid.super());
  }

  WorkBranch& GetRef(Bid bid) {
    return *(bid.branch());
  }

  Supervisor& GetRef(Sid sid) {
    return *(sid.super());
  }

  template<typename T = cos::base::normal, typename F,
           typename R = cos::base::result_of_t<F>,
           typename DR = typename std::enable_if<std::is_void<R>::value>::type>
  void Submit(F&& task) {
    assert(!branches_list_.empty());
    auto this_branch = current_->get();
    auto next_branch = Forward(current_)->get();
    if (next_branch->TasksNum() < this_branch->TasksNum()) {
      next_branch->Submit<T>(std::forward<F>(task));
    } else {
      this_branch->Submit<T>(std::forward<F>(task));
    }
  }
  
  template<typename T = cos::base::normal, typename F,
           typename R = cos::base::result_of_t<F>,
           typename DR = typename std::enable_if<!std::is_void<R>::value>::type>
  auto Submit(F&& task) -> std::future<R> {
    assert(!branches_list_.empty());
    auto this_branch = current_->get();
    auto next_branch = Forward(current_)->get();
    if (next_branch->TasksNum() < this_branch->TasksNum()) {
      return next_branch->Submit<T>(std::forward<F>(task));
    } else {
      return this_branch->Submit<T>(std::forward<F>(task));
    }
  }

  template <typename T, typename F, typename... Fs>
  auto Submit(F&& task, Fs&&... tasks) 
      -> typename std::enable_if<std::is_same<T, cos::base::sequence>::value>::type {
    assert(!branches_list_.empty());
    auto this_branch = current_->get();
    auto next_branch = Forward(current_)->get();
    if (next_branch->TasksNum() < this_branch->TasksNum()) {
      return next_branch->Submit<T>(std::forward<F>(task), std::forward<Fs>(tasks)...);
    } else {
      return this_branch->Submit<T>(std::forward<F>(task), std::forward<Fs>(tasks)...);
    }
  }

  private:
   const pos_t& Forward(pos_t& current) {
    if (++current == branches_list_.end()) {
      current = branches_list_.begin();
    }
    return current;
   }

   BranchList branches_list_;
   SupervisorMap supers_map_;
   pos_t current_ = {};
};

}  // namespace workspace
}  // namespace cos

#endif  // WORKSPACE_WORKSPACE_H_