#pragma once

#include "scenario.h"
#include "MiLi\mili.h"
#include "mili_helpers.h"
#include <iostream>

using namespace mili;
using namespace std;


struct MiliTask : Task, mili::Coroutine {

  int run() {
    BEGIN_COROUTINE
    while (true) {
      while (!worker.atMine()) {
        worker.moveMine();
        mili_yield(0);
      }
      do {
        worker.gather();
        mili_yield(0);
      } while (worker.isMining());
      while (!worker.atHome()) {
        worker.moveHome();
        mili_yield(0);
      }
      worker.dropoff();
      mili_yield(0);
    }
    END_COROUTINE(0);
  }

  void print(ostream& stream) const { 
    stream << "Mili Coroutine1: " << worker.total << endl;
  }
};




inline int runMili() {
  cout << "Raw MiLi coroutines" << endl;
  auto *task = new MiliTask();
  World world(task);
  return world.run();
}