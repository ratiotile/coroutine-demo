#pragma once
#include "scenario.h"
#include "MiLi\mili.h"
#include "mili_helpers.h"
#include <iostream>
#include <queue>

using namespace mili;
using namespace std;

struct MiliTask2;

static MiliTask2* next_frame2;
static MiliTask2* end_coro2;

struct MiliTask2 : mili::Coroutine {
  MiliTask2* caller = nullptr;
  virtual MiliTask2* run() {
    return end_coro2;
  }
};

struct MiliTask2_Worker : MiliTask2 {
  Worker& worker;
  MiliTask2_Worker(Worker& p_worker) :worker(p_worker) {}
};


struct MiliTask2Main : MiliTask2_Worker {
  virtual MiliTask2* run() {
    BEGIN_COROUTINE
      while (true) {
        while (!worker.atMine()) {
          worker.moveMine();
          mili_yield(next_frame2);
        }
        do {
          worker.gather();
          mili_yield(next_frame2);
        } while (worker.isMining());
        while (!worker.atHome()) {
          worker.moveHome();
          mili_yield(next_frame2);
        }
        worker.dropoff();
        mili_yield(next_frame2);
      }
    END_COROUTINE(end_coro2);
  }
  MiliTask2Main(Worker& p_worker) :MiliTask2_Worker(p_worker) {}
};

struct MiliTask2Mgr : Task {
  queue<MiliTask2*> q;
  int run() {
    do {
      auto curr = q.front();
      q.pop();
      auto awt = curr->run();
      if (awt == next_frame2) {
        q.push(curr);
        break;
      }
    } while (q.size() > 0);
    return 0;
  }
  MiliTask2Mgr()
    : Task()
  {
    auto t = new MiliTask2Main(worker);
    q.push(t);
  }

  void print(ostream& stream) const {
    stream << "Mili Coroutine2: " << worker.total << endl;
  }
};


inline int runMili2() {
  cout << "MiLi coroutines with managed queue" << endl;
  next_frame2 = new MiliTask2();
  end_coro2 = new MiliTask2();
  auto *task = new MiliTask2Mgr();
  World world(task);
  return world.run();
}