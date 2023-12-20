
#include <thread>

#include "gtest/gtest.h"
#include "workspace.h"

using cos::workspace::Workspace;
using cos::workspace::WorkBranch;
using cos::workspace::Supervisor;
#define TID() std::this_thread::get_id()

TEST(Workspace, base) {
  Workspace space;
  auto b1 = space.Attach(new WorkBranch(2));
  auto b2 = space.Attach(new WorkBranch(2));
  auto sp = space.Attach(new Supervisor(2, 4, 1000));
  
  if (b1 != b2) 
      std::cout << "b1["<<b1<<"] != b2[" << b2 << "]" << std::endl;
  if (b1 < b2) 
      std::cout << "b1[" << b1 << "] <  b2[" << b2 << "]" << std::endl;
  space[sp].Supervise(space[b1]);
  space[sp].Supervise(space[b2]);
  
  // nor task A and B
  space.Submit([]{std::cout << TID() << " exec task A"<<std::endl;});   
  space.Submit([]{std::cout << TID() << " exec task B"<<std::endl;});
  
  // wait for tasks done
  space.ForEach([](WorkBranch& each){ each.WaitTasks(); });
  
  // Detach one workbranch and there remain one.
  auto br = space.Detach(b1); 
  std::cout<<"workspace still maintain: ["<<b2<<"]"<<std::endl;
  std::cout<<"workspace no longer maintain: ["<<b1<<"]"<<std::endl;
  
  auto& ref = space.GetRef(b2);
  ref.Submit<cos::base::normal>([]{std::cout << TID() << " exec task C"<<std::endl;});
  
  // wait for tasks done
  space.ForEach([](WorkBranch& each){ each.WaitTasks(); });
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}