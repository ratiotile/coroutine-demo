#undef _HAS_STD_BYTE

#include "3 MiLi await.hpp"
#include "coroutines_ts.h"

int __cdecl main() {
  //int score = 0;
  cout << "__Coroutine Comparison__\n" << endl;
  //runMili3();

  //runMili();
  //runMili2();

  //cout << "testing simple coroutines with std::future\n";
  //future_main();

  cout << "testing Coroutines TS tasks: \n";
  cts::run_cts_example();
  return 0;
}