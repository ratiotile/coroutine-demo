#include "coroutines_ts.h"
#include <assert.h>
#include <future>
#include<experimental/coroutine>


namespace cts {
TaskManager* Task::tm = nullptr;


Task doWork(Worker & worker) {
  assert(worker.atMine());
  do {
    worker.gather();
    co_await Task::tm->sleepFrames();
  } while (worker.isMining());
  co_return;
}

Task goToMine(Worker & worker) {
  assert(!worker.atMine());
  std::cout << "go to mine";
  while(!worker.atMine()) {
    worker.moveMine();
    co_await doWork(worker);
  }
  co_return;
}

Task onFrame() {
  while(true) {
    std::cout << "onframe\n";
    co_await Task::tm->sleepFrames();
  }
}


/*****************************************************************************/
MyCoro test_fun(MyFuture& fut) {
  cout << "t waiting on future " << &fut << "\n";
  co_await fut;
  cout << "t done\n";
}

void benchmark_cts_tasks() {
  cout << "creating fut\n";
  MyFuture fut;
  auto t = test_fun(fut);
  cout << "t created\n";
  auto const t2 = [](MyCoro& t) -> MyCoro {
    cout << "t2 awaiting on t " << &t << "\n";
    co_await t;
    cout << "t2 done!\n";
  };
  cout << "t2 paused\n";
  [[maybe_unused]] auto temp = t2(t);
  cout << "setting future ready\n";
  fut.runTasks();

  return;
  TaskManager tm{};
  Task::tm = &tm;
  onFrame();
  cout << "next\n";
  tm.nextFrame();
  cout << "next\n";
  tm.nextFrame();
}

}
