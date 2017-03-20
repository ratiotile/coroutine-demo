#include<iostream>
#include<string>
#include<experimental/generator>
#include<experimental/coroutine>
namespace stdx = std::experimental;
using stdx::coroutine_handle;
using std::cout;


struct ResumableThing {
  struct promise_type {
    // factory function
    ResumableThing get_return_object() {
      return ResumableThing(coroutine_handle<promise_type>::from_promise(*this));
    }
    auto initial_suspend() { return stdx::suspend_never(); }
    auto final_suspend() { return stdx::suspend_never(); }
    void return_void() {}
  };
  coroutine_handle<promise_type> _coroutine = nullptr;
  explicit ResumableThing(coroutine_handle<promise_type> coroutine)
    : _coroutine(coroutine) {
  }
  ~ResumableThing() {
    if (_coroutine) _coroutine.destroy();
  }
  ResumableThing() = default;
  ResumableThing(ResumableThing const&) = delete; // no copy
  ResumableThing& operator=(ResumableThing const&) = delete; // no copy assign
  ResumableThing(ResumableThing&& other) // move constructor
    : _coroutine(other._coroutine) {
    other._coroutine = nullptr;
  }
  ResumableThing& operator=(ResumableThing&& other) { // move assign
    if (&other != this) {
      _coroutine = other._coroutine;
      other._coroutine = nullptr;
    }
  }
  auto resume() { _coroutine.resume(); }
};

ResumableThing countCoroutine() {
  cout << "count called\n";
  for (int i = 0; ; ++i) {
    co_await stdx::suspend_always();
    cout << "count resumed\n";
  }
}

ResumableThing namedCounter(std::string name) {
  cout << "counter(" << name << ") was called\n";
  for (int i = 1; ; ++i) {
    co_await stdx::suspend_always();
    cout << "counter(" << name << ") resumed #" << i << "\n";
  }
}

int foo() {
  ResumableThing c = countCoroutine();
  c.resume();
  ResumableThing a = namedCounter("a");
  ResumableThing b = namedCounter("b");
  a.resume();
  b.resume();
  b.resume();
  a.resume();
  return 0;
};