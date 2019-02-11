#include "coroutines_ts.h"
#include <assert.h>


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

void benchmark_cts_tasks() {
  TaskManager tm{};
  Task::tm = &tm;
  onFrame();
  cout << "next\n";
  tm.nextFrame();
  cout << "next\n";
  tm.nextFrame();
}

}
