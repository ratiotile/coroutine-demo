#pragma once
#include<experimental/coroutine>
#include<iostream>
#include <gsl.h>

namespace cts {
using std::cout;
using std::experimental::coroutine_handle;
using std::experimental::suspend_always;
using std::experimental::suspend_never;

/**
 * Generic Future that multiple coroutines may co_await
 */
struct MyFuture {
  struct awaiter {
    MyFuture & m_future;
    awaiter* m_next{nullptr}; // linked list
    coroutine_handle<> m_awaiter;
    awaiter(MyFuture & future) : m_future(future) {
      // cout << "constructor ";
      // printStats();
    }
    ~awaiter() {
      // cout << "destructor ";
      // printStats();
    }
    bool await_ready() const noexcept { return m_future.is_ready; }
    bool await_suspend(coroutine_handle<void> awaiting_task) noexcept {
      m_awaiter = awaiting_task;
      if(m_future.is_ready) return false; // run right away
      // cout << "await suspend on " << awaiting_task.address() << " ";
      // printStats();
      m_next = m_future.m_list_head;
      m_future.m_list_head = this;
      m_future.printStats();
      return true;
    }
    void await_resume() const noexcept {
      // cout << "await resume ";
      // printStats();
    }
    void printStats() const {
      cout << "MyFuture::awaiter[" << this << "] -> " << m_awaiter.address() 
           << "\n";
    }
  };

  bool is_ready{false};
  awaiter* m_list_head{nullptr};
  MyFuture() {
    // cout << "constructor ";
    // printStats();
  };
  MyFuture(MyFuture const& by_copy) = delete;
  MyFuture(MyFuture && by_move) = delete;
  ~MyFuture() {
    // cout << "Destructor ";
    // printStats();
  }
  MyFuture& operator=(MyFuture const& copy) = delete;
  MyFuture& operator=(MyFuture && move) = delete;

  awaiter operator co_await() noexcept {
    // cout << "co_await: ";
    // printStats();
    return awaiter {*this};
  }
  void runTasks() {
    is_ready = true;
    // cout << "runTasks start ";
    // printStats();
    auto current = m_list_head;
    while (current != nullptr) {
      auto const next = current->m_next;
      // cout << "resuming " << current->m_awaiter.address() << "\n";
      current->m_awaiter.resume();
      current = next;
    }
    m_list_head = current;
    // cout << "runTasks done ";
    // printStats();
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
      // cout << "construct "; printStats();
    }
    ~promise_type() {
      // cout << "destruct "; printStats();
    }
    MyCoro get_return_object() {
      auto coro = MyCoro{coroutine_handle<promise_type>::from_promise(*this)};
      // just shows destructor immediately return by value
      // cout << "get_return_object = "; coro.printStats(); 
      // cout << "  from " ; printStats();
      return coro;
    }
    auto initial_suspend() {
      // cout << "initial_suspend "; printStats();
      return suspend_never();
    }
    auto final_suspend() {
      // cout << "final_suspend "; printStats();
      return suspend_never();
    }
    // last chance to resume the awaiting coroutine?
    void return_void() {
      // cout << "return_void "; printStats();
      if (m_continuation) {
        // cout << "  resuming\n";
        m_continuation.resume();
      }
    }
    void unhandled_exception() {}
    void set_continuation( coroutine_handle<> coro) {
      m_continuation = coro;
      // cout << "set_continuation "; printStats();
    }
    void printStats() {
      cout << "promise_type[" << this << "] = " << m_continuation.address() << "\n";
    }
  };

  struct awaiter {
    coroutine_handle<promise_type> m_coro;
    awaiter(coroutine_handle<promise_type> coro) : m_coro(coro) {
      // cout << "construct ";
      // printStats();
    }
    ~awaiter() {
      // cout << "destruct ";
      // printStats();
    }
    // is it ready to run?
    bool await_ready() const noexcept { return !m_coro || m_coro.done(); }
    // suspends the caller, takes the handle of the
    // coroutine which is executing the co_await, so that it can be resumed
    // when ready.
    void await_suspend(coroutine_handle<promise_type> awaiting_coro) noexcept {
      m_coro.promise().set_continuation(awaiting_coro);
      // cout << "await_suspend on " << awaiting_coro.address() << " ";
      // printStats();
    }
    // get value to return when done
    void await_resume() noexcept {
      // cout << "await_resume ";
      // printStats();
    }
    void printStats() {
      cout << "MyCoro::awaiter[" << this << "] -> " << m_coro.address() << "\n";
    }
  };

  coroutine_handle<promise_type> m_coroutine;  
  explicit MyCoro(coroutine_handle<promise_type> coro) : m_coroutine(coro) {
    // cout << "constructor ";
    // printStats();
  }
  MyCoro(MyCoro const& by_copy) = delete;
  MyCoro(MyCoro && to_move) = default;
  MyCoro& operator=(MyCoro const& by_copy) = delete;
  MyCoro& operator=(MyCoro && to_move) = default;
  ~MyCoro() {
    // cout << "destructor ";
    // printStats();
  }

  auto operator co_await() {
    // cout << "co_await: ";
    // printStats();
    return awaiter{m_coroutine};
  }
  void resume() {
    // cout << "resume ";
    // printStats();
    m_coroutine.resume();
  }
  void printStats() {
    cout << "MyCoro[" << this << "] -> " << m_coroutine.address() << "\n";
  }
};

void run_cts_example();
}