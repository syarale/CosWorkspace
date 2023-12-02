
#include <thread>
#include "gtest/gtest.h"
#include "autothread.h"
#include "thread_safe_queue.h"

using cos::base::AutoThread;
using cos::base::ThreadSafeQueue;
using cos::base::join;

TEST(ThreadSafeQueueTest, PushPop) {
  ThreadSafeQueue<int> que;
  int bound = 10000000;
  
  std::thread thrd1([&que, bound] {
    for (int i = 0; i < bound; i++) {
      que.push_back(i);
    }
  });
  AutoThread<join> athrd1(std::move(thrd1));

  std::thread thrd2([&que, bound] {
    for (int i = bound; i < bound * 2; i ++) {
      que.push_front(i);
    }
  });
  AutoThread<join> athrd2(std::move(thrd2));

  std::thread thrd3([&que, bound] {
    std::size_t size = 2 * bound;
    std::size_t count = 0;
    while (count < size) {
      int tmp = 0;
      if (que.try_pop(tmp)) {
        count ++;
      }
    }
    EXPECT_TRUE(que.empty());
  });
  AutoThread<join> athrd3(std::move(thrd3));
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
