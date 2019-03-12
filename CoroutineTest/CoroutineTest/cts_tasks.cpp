#include "cts_tasks.h"
#include <cassert>

namespace cts {
TaskManager* TaskManager::instance = nullptr;

struct WorkerTask : Task {  
  MyCoro run() override {
    while(true) {
      cout << "running \n";
      co_await TaskManager::instance->sleepFrames();
    }
  }

  void cancel() override {
    
  }
};

void cts_task_benchmark() {
  TaskManager::instance = new TaskManager{};
  WorkerTask t;
  TaskManager::instance->addTask(t.run());
  cout << "next frame\n";
  TaskManager::instance->nextFrame();
  // test cancelling
  TaskManager::instance->cancelAll();
  cout << "next frame\n";

  TaskManager::instance->nextFrame();
}
}
