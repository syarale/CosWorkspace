
#include <thread>

#include "gtest/gtest.h"
#include "workbranch.h"

using cos::workspace::WorkBranch;

TEST(WorkBranch, add_and_remove) {
  WorkBranch workers_pool(1000);
  EXPECT_EQ(workers_pool.WorkersNum(), 1000);

  std::thread t1([&workers_pool] {
    for (int i = 0; i < 2000; i ++) {
      workers_pool.AddWorker();
    }
  });
  
  std::thread t2([&workers_pool] {
    for (int i = 0; i < 1000; i ++) {
      workers_pool.RemoveWorker();
    }
  });
  workers_pool.WaitTasks();
  
  t1.join();
  t2.join();
  EXPECT_EQ(workers_pool.WorkersNum(), 2000);
}

TEST(WorkBranch, submit) {
  WorkBranch workers_pool(2);
  
  int x1, x2, x3, x4, x5, x6;
  x1 = x2 = x3 = x4 = x5 = x6 = 1;
  workers_pool.Submit([&x1]{ x1 = 10; });
  workers_pool.Submit<cos::base::normal>([&x2]{ x2 = 10; });
  workers_pool.Submit<cos::base::urgent>([&x3]{ x3 = 10; });
  workers_pool.Submit<cos::base::sequence>([&x4]{ x4 = 10; },
                                           [&x5]{ x5 = 10; },
                                           [&x6]{ x6 = 10; });
  sleep(1);
  EXPECT_EQ(x1, 10);
  EXPECT_EQ(x2, 10);
  EXPECT_EQ(x3, 10);
  EXPECT_EQ(x4, 10);
  EXPECT_EQ(x5, 10);
  EXPECT_EQ(x6, 10);

  std::future<int> res1 = workers_pool.Submit([]{
    return 10;
  });
  std::future<int> res2 = workers_pool.Submit<cos::base::urgent>([]{
    return 100;
  });
  EXPECT_EQ(res1.get(), 10);
  EXPECT_EQ(res2.get(), 100);
  sleep(1);
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
