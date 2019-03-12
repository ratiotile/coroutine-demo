#pragma once
#include<experimental/coroutine>
#include<iostream>
#include<algorithm>
#include<array>
#include <vector>
#include "coroutines_ts.h"
#include "scenario.h"
#include "gsl-lite.hpp"


namespace cts {
using std::cout;
using std::experimental::coroutine_handle;
using std::experimental::suspend_always;
using std::experimental::suspend_never;

struct TaskManager;
struct Task {
  gsl::not_null<TaskManager*> m_manager;
  explicit Task(gsl::not_null<TaskManager*> manager): m_manager(manager) {}
  virtual ~Task() = default;
  virtual MyCoro run() = 0;
  virtual void cancel() = 0;
};

// need to be able to cancel task and reassign units
struct TaskUnits {
  MyCoro m_coro;
  explicit TaskUnits(MyCoro && coro)
  : m_coro(std::move(coro)) {}
};

struct TaskManager {
  static TaskManager* instance;
  static constexpr auto max_frames = 5;
  MyFuture m_frames[max_frames];
  int m_index = 0;
  std::vector<TaskUnits> m_tasks;

  void addTask(MyCoro&& coro) {
    m_tasks.emplace_back(std::move(coro));
  }

  int getIndex(int n) const {
    return (m_index + n) % max_frames;
  }

  void nextFrame() {
    cout << "next frame\n";
    ++m_index;
    auto& frame = gsl::at(m_frames, m_index);
    frame.runTasks();
  }

  // wait for n frames
  MyFuture & sleepFrames(int n = 1) {
    const auto i = getIndex(n);
    cout << "awaiting frame index " << i << std::endl;
    auto & frame = m_frames[i];//gsl::at(m_frames, i);//
    return frame;
  }

  void cancelAll() {
    for(auto & tu : m_tasks) {
      cout << "cancelling "; tu.m_coro.printStats();
      for(auto & f : m_frames) {
        if(f.m_list_head) {
          cout << "cancel2 "; f.printStats();
          f.cancel(tu.m_coro.m_coroutine.promise().m_continuation);
        }
      }
      tu.m_coro.cancel(); // delete coroutine frame
    }
    m_tasks.clear();
    cout << "all tasks cancelled!\n";
  }
};

void cts_task_benchmark();
}
