#pragma once
#include "scenario.h"
#include<experimental/coroutine>
#include<iostream>
#include<algorithm>
#include<array>
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




/******************************************************************************/
/**
 * Generic Future that multiple coroutines may co_await
 */
struct MyFuture {
  struct awaiter {
    MyFuture & m_future;
    awaiter* m_next{nullptr}; // linked list
    coroutine_handle<> m_awaiter;
    awaiter(MyFuture & future) : m_future(future) {
      cout << "constructor ";
      printStats();
    }
    ~awaiter() {
      cout << "destructor ";
      printStats();
    }
    bool await_ready() const noexcept { return m_future.is_ready; }
    bool await_suspend(coroutine_handle<void> awaiting_task) noexcept {
      m_awaiter = awaiting_task;
      if(m_future.is_ready) return false; // run right away
      cout << "await suspend on " << awaiting_task.address() << " ";
      printStats();
      m_next = m_future.m_list_head;
      m_future.m_list_head = this;
      m_future.printStats();
      return true;
    }
    void await_resume() const noexcept {
      cout << "await resume ";
      printStats();
    }
    void printStats() const {
      cout << "MyFuture::awaiter[" << this << "] -> " << m_awaiter.address() 
           << "\n";
    }
  };

  bool is_ready{false};
  awaiter* m_list_head{nullptr};
  MyFuture() {
    cout << "constructor ";
    printStats();
  };
  ~MyFuture() {
    cout << "Destructor ";
    printStats();
  }

  awaiter operator co_await() noexcept {
    cout << "co_await: ";
    printStats();
    return awaiter {*this};
  }
  void runTasks() {
    is_ready = true;
    cout << "runTasks start ";
    printStats();
    auto current = m_list_head;
    while (current != nullptr) {
      auto const next = current->m_next;
      cout << "resuming " << current->m_awaiter.address() << "\n";
      current->m_awaiter.resume();
      current = next;
    }
    m_list_head = current;
    cout << "runTasks done ";
    printStats();
  }
  
  void printStats() {
    cout << "MyFuture[" << this << "]";
    auto current = m_list_head;
    while (current != nullptr) {
      cout << " -> " << current;
      current = current->m_next;
    }
    cout << "\n";
  }
};

/**
 * General-purpose Coroutine class that can both awaited on and resumed
 */
struct MyCoro {
  struct promise_type {
    coroutine_handle<> m_continuation;
    promise_type() {
      cout << "construct "; printStats();
    }
    ~promise_type() {
      cout << "destruct "; printStats();
    }
    MyCoro get_return_object() {
      auto coro = MyCoro{coroutine_handle<promise_type>::from_promise(*this)};
      // just shows destructor immediately return by value
      //cout << "get_return_object = "; coro.printStats(); 
      //cout << "  from " ; printStats();
      return coro;
    }
    auto initial_suspend() {
      cout << "initial_suspend "; printStats();
      return suspend_never();
    }
    auto final_suspend() {
      cout << "final_suspend "; printStats();
      return suspend_always();
    }
    // last chance to resume the awaiting coroutine?
    void return_void() {
      cout << "return_void "; printStats();
      if (m_continuation) {
        cout << "  resuming\n";
        m_continuation.resume();
      }
    }
    void unhandled_exception() {}
    void set_continuation( coroutine_handle<> coro) {
      m_continuation = coro;
      cout << "set_continuation "; printStats();
    }
    void printStats() {
      cout << "promise_type[" << this << "] = " << m_continuation.address() << "\n";
    }
  };

  struct awaiter {
    coroutine_handle<promise_type> m_coro;
    awaiter(coroutine_handle<promise_type> coro) : m_coro(coro) {
      cout << "construct ";
      printStats();
    }
    ~awaiter() {
      cout << "destruct ";
      printStats();
    }
    // is it ready to run?
    bool await_ready() const noexcept { return !m_coro || m_coro.done(); }
    // suspends the caller, takes the handle of the
    // coroutine which is executing the co_await, so that it can be resumed
    // when ready.
    void await_suspend(coroutine_handle<promise_type> awaiting_coro) noexcept {
      m_coro.promise().set_continuation(awaiting_coro);
      cout << "await_suspend on " << awaiting_coro.address() << " ";
      printStats();
    }
    // get value to return when done
    void await_resume() noexcept {
      cout << "await_resume ";
      printStats();
    }
    void printStats() {
      cout << "MyCoro::awaiter[" << this << "] -> " << m_coro.address() << "\n";
    }
  };

  coroutine_handle<promise_type> m_coroutine;  
  MyCoro(coroutine_handle<promise_type> coro) : m_coroutine(coro) {
    cout << "constructor ";
    printStats();
  }
  ~MyCoro() {
    cout << "destructor ";
    printStats();
  }
  auto operator co_await() {
    cout << "co_await: ";
    printStats();
    return awaiter{m_coroutine};
  }
  void resume() {
    cout << "resume ";
    printStats();
    m_coroutine.resume();
  }
  void printStats() {
    cout << "MyCoro[" << this << "] -> " << m_coroutine.address() << "\n";
  }
};

void benchmark_cts_tasks();
}