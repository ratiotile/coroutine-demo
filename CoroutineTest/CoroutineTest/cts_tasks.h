#pragma once
#include<experimental/coroutine>
#include<iostream>
#include<algorithm>
#include<array>
#include <vector>
#include "coroutines_ts.h"
#include "scenario.h"
#include <gsl.h>


namespace cts {
using std::cout;
using std::experimental::coroutine_handle;
using std::experimental::suspend_always;
using std::experimental::suspend_never;

struct Task {
  virtual ~Task() = default;
  virtual MyCoro run() = 0;
  virtual void cancel() = 0;
};

struct TaskManager {
  static TaskManager* instance;
  static constexpr auto max_frames = 5;
  MyFuture m_frames[max_frames];
  int m_index = 0;
  // std::vector<Task> m_tasks;

  // void addTask(Task&& task) {
  //   m_tasks.emplace_back(std::move(task));
  // }

  int getIndex(int n) const {
    return (m_index + n) % max_frames;
  }

  void nextFrame() {
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
};

void cts_task_benchmark();
}
