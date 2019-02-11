#pragma once
#include "scenario.h"
#include<experimental/coroutine>
#include<iostream>
#include <vector>
#include <gsl.h>

namespace cts {
using std::cout;
using std::experimental::coroutine_handle;
using std::experimental::suspend_always;
using std::experimental::suspend_never;

struct TaskManager;

struct Task {
  struct promise_type {
    Task get_return_object() {
      return Task(coroutine_handle<promise_type>::from_promise(*this));
    }
    auto initial_suspend() { return suspend_never(); }
    auto final_suspend() { return suspend_always(); }
    void return_void() {}
    void unhandled_exception() {}
  };
  struct awaiter {
    Task & m_task;
    awaiter(Task & t) : m_task(t) {}
    // is it ready to run?
    bool await_ready() const noexcept { return m_task.is_done; }
    // suspends the caller, takes the handle of the
    // coroutine which is executing the co_await, so that it can be resumed
    // when ready.
    bool await_suspend(coroutine_handle<void> awaiting_task) noexcept {
      if(m_task.is_done) return false; // false to immediately resume caller.
      //m_task.addTask(awaiting_task);
      return true; // do not resume caller
    }
    // get value to return when done
    void await_resume() noexcept {}
  };
  static TaskManager* tm;
  bool is_done = false;
  coroutine_handle<promise_type> m_coroutine;
  explicit Task(coroutine_handle<promise_type> coroutine)
    : m_coroutine(coroutine) {
  }
  Task() = default;
  Task(Task const&) = delete; // no copy
  Task& operator=(Task const&) = delete; // no copy assign
  // move constructor
  Task(Task&& other) noexcept: m_coroutine(other.m_coroutine) {
    other.m_coroutine = nullptr;
  }
  // move assign
  Task& operator=(Task&& other) noexcept {
    if (&other != this) {
      m_coroutine = other.m_coroutine;
      other.m_coroutine = nullptr;
    }
    return *this;
  }
  ~Task() {
    if(m_coroutine) m_coroutine.destroy();
  }
  awaiter operator co_await() noexcept {
    awaiter a(*this);
    return a;
  }
  void resume() const {m_coroutine.resume();}
};

// many Tasks may wait on a future frame to be ready
struct FrameEvent {
  struct awaiter {
    FrameEvent & m_event;
    awaiter(FrameEvent & frame) : m_event(frame) {}
    // is it ready to run?
    bool await_ready() const noexcept { return m_event.is_ready; }
    // suspends the caller, takes the handle of the
    // coroutine which is executing the co_await, so that it can be resumed
    // when ready.
    bool await_suspend(coroutine_handle<void> awaiting_task) noexcept {
      if(m_event.is_ready) return false; // false to immediately resume caller.
      m_event.addAwaitingTask(awaiting_task);
      return true; // do not resume caller
    }
    // get value to return when done
    void await_resume() noexcept {}
  };

  bool is_ready = false;
  std::vector<coroutine_handle<void>> m_waiting_tasks;
  FrameEvent() = default;
  // allow await on
  awaiter operator co_await() const noexcept {
    cout << "FrameEvent co_await\n";
    auto awaiter{*this};
    return awaiter;
  }
  // add task to be resumed when this frame is ready
  void addAwaitingTask(coroutine_handle<void> task) {
    m_waiting_tasks.emplace_back(task);
    cout << "addAwaitingTask size=" << m_waiting_tasks.size() << "\n";
  }
  // run all waiting tasks
  void runTasks() {
    is_ready = true;
    cout << "runTasks set ready, there are " << m_waiting_tasks.size() << " tasks waiting.\n";
    // list could change during iteration
    for(auto iter = m_waiting_tasks.begin(); iter != m_waiting_tasks.end(); ++iter) {  // NOLINT(modernize-loop-convert)
      iter->resume();
    }
    m_waiting_tasks.clear();
  }
  void reset() {
    is_ready = false;
    m_waiting_tasks.clear();
  }
};

struct TaskManager {
  static constexpr auto max_frames = 5;
  FrameEvent m_frames[max_frames];
  int m_index = 0;
  std::vector<Task> m_tasks;

  void addTask(Task&& task) {
    m_tasks.emplace_back(std::move(task));
  }

  int getIndex(int n) const {
    return (m_index + n) % max_frames;
  }

  void nextFrame() {
    auto frame = gsl::at(m_frames, m_index);
    frame.runTasks();
    ++m_index;
  }

  // wait for n frames
  FrameEvent & sleepFrames(int n = 1) {
    auto const i = getIndex(n);
    cout << "awaiting frame index " << i << std::endl;
    auto & frame = gsl::at(m_frames, i);//m_frames[i];
    return frame;
  }
};

void benchmark_cts_tasks();
}
