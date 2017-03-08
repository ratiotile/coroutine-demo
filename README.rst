C++ Coroutines in MSVC 2015
===========================

[n4134]_ - Resumable Functions rev2 was implemented in VS2015 Update 2 [#]_

It appears that Visual Studio 2015 Update 3 actually implements http://open-std.org/JTC1/SC22/WG21/docs/papers/2015/p0057r0.pdf

an awaitable type needs to define 3 member functions:

.. code::
  bool await_ready() const;
  void await_suspend(std::coroutine_handle<>);
  T await_resume();

await_ready() is called when co_await is used on an awaitable. It returns false to suspend, otherwise it needs to have a value ready.

await_suspend() is called when await_ready() returns false, right before the function is suspended.

await_resume() is called right before this function is resumed.

The coroutine promise is looked up under the resumable function type R::promise_type
A resumable function must specify coroutine promise, which has:

.. code::
  get_return_object()
  initial_suspend()
  final_suspend()

Coroutines must be of type resumable function.

.. code::
  struct promise_type;
  constructor(coroutine_handle<promise_type> c);
  destructor(); // call c.destroy()


.. [n4134] http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4134.pdf

.. [#] https://github.com/kirkshoop/await
