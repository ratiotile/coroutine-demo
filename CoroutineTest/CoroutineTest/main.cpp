#include "test_scenario.h"
#include "1 MiLi coroutine.hpp"
#include "2 MiLi queue.hpp"
#include "3 MiLi await.hpp"



int main() {
  int score = 0;
  cout << "__Coroutine Comparison__\n" << endl;
  score += runMili();
  score += runMili2();
  score += runMili3();
  return score;
}