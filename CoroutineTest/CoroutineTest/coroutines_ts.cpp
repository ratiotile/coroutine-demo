#include "coroutines_ts.h"

namespace cts {
MyCoro test_fun(MyFuture& fut) {
  cout << "t waiting on future " << &fut << "\n";
  /* c1 = coroutine_handle
   * f_a = MyFuture::awaiter(fut) 
   * f_a.await_suspend(c1)
   *   f_a.m_awaiter = c1
   *   fut.m_list_head = f_a
   */
  co_await fut;
  cout << "t done\n";
  /* c_p.return_void()
   * c_h.resume()
   */
}

void run_cts_example() {
  cout << "creating fut\n";
  MyFuture fut;
  cout << "run first coroutine\n";
  auto t = test_fun(fut);
  cout << "t created\n";
  auto const t2 = [](MyCoro& t) -> MyCoro {
    cout << "t2 awaiting on t " << &t << "\n";
    /* c_p = MyCoro::promise_type(t)
     * c_a = MyCoro::awaiter(t)
     * c_h = coroutine_handle(t2)
     * c1.await_suspend(c_h)
     *   c_p.m_continuation = c_h
     */
    co_await t;
    /* c_a.await_resume()
     */
    cout << "t2 done!\n";
  };
  cout << "run second coroutine\n";
  [[maybe_unused]] auto temp = t2(t);
  cout << "setting future ready\n";
  fut.runTasks();
  /* f_a.resume()
   * f_a.await_resume()
   */
  return;

}

}
