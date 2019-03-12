#include "cts_tasks.h"
#include <cassert>

namespace cts {

struct WorkerTask : Task {  
  WorkerTask(gsl::not_null<TaskManager*> manager) : Task(manager) {}
  MyCoro run() override {
    while(true) {
      cout << "running \n";
      co_await m_manager->sleepFrames();
    }
  }

  void cancel() override {
    
  }
};

void cts_task_benchmark() {
  auto tm = TaskManager{};
  WorkerTask t(&tm);
  tm.addTask(t.run());
  tm.nextFrame();
  // test cancelling
  tm.cancelAll();

  tm.nextFrame();
  cout << "shouldn't have run \n";
}
}
