
#include <thread>

#include "gtest/gtest.h"
#include "supervisor.h"
#include "workbranch.h"

using cos::workspace::WorkBranch;
using cos::workspace::Supervisor;

TEST(Supervisor, supervise) {
    WorkBranch br1(2);
    WorkBranch br2(2);

    // 2 <= thread number <= 8 
    // time interval: 1000 ms 
    Supervisor sp(2, 8, 1000);

    sp.SetTickCallback([&br1, &br2]{
        auto now = std::chrono::system_clock::now();
        std::time_t timestamp = std::chrono::system_clock::to_time_t(now);
        std::tm local_time = *std::localtime(&timestamp);
        static char buffer[40];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &local_time);
        std::cout<<"["<<buffer<<"] "<<"br1: [workers] "<<br1.WorkersNum()<<" | [blocking-tasks] "<<br1.TasksNum()<<'\n';
        std::cout<<"["<<buffer<<"] "<<"br2: [workers] "<<br2.WorkersNum()<<" | [blocking-tasks] "<<br2.TasksNum()<<'\n';
    });

    sp.Supervise(br1);  // start supervising
    sp.Supervise(br2);  // start supervising

    for (int i = 0; i < 1000; ++i) {
        br1.Submit([]{std::this_thread::sleep_for(std::chrono::milliseconds(10));});
        br2.Submit([]{std::this_thread::sleep_for(std::chrono::milliseconds(20));});
    }

    br1.WaitTasks();
    br2.WaitTasks();
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
