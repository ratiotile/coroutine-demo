#pragma once
#include "scenario.h"
#include <iostream>
#include <chrono>
#include <string>

struct Task {
  Worker worker;
  virtual int run() = 0;
  virtual void print(ostream& stream) const {
    stream << "overload this" << endl;
  }
  virtual ~Task() = default;
};

inline ostream& operator<<(ostream& stream, Task const & task) {
  task.print(stream);
  return stream;
}

struct TaskManager {
  std::vector<Task*> m_tasks;

  void addTask(Task* t) {
    m_tasks.push_back(t);
  }

  void cleanup() {
    for(auto t : m_tasks) {
      delete t;
    }
    m_tasks.clear();
  }
};

struct World {
  int frame = 0;
  Task* task;

  void nextFrame() {
    frame++;
    task->run();
  }

  int run() {
    auto const start = chrono::high_resolution_clock::now();
    for (auto i = 0; i < frames_to_run; ++i) 
    {
      nextFrame();
    }
    auto const endt = chrono::high_resolution_clock::now();
    auto const diff = endt - start;
    return end(diff);
  }

  int end(chrono::nanoseconds ns) {
    cout << "Completed trips: " << *task << " Time: " << ns.count() << endl;
    auto const score = task->worker.total;
    delete task;
    return score;
  }

  World(Task* t) {
    task = t;
  }
};
