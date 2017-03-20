#pragma once
#include "test_scenario.h"
#include "MiLi\mili.h"
#include <iostream>
#include <queue>

using namespace mili;
using namespace std;

struct MiliTask3;

static MiliTask3* next_frame3;
static MiliTask3* end_coro3;

struct MiliTask3 : mili::Coroutine {
  MiliTask3* caller = nullptr;
  virtual MiliTask3* run() {
    return end_coro3;
  }
};

struct MiliTask3_Worker : MiliTask3 {
  Worker& worker;
  MiliTask3_Worker(Worker& p_worker) :worker(p_worker) {}
};

struct MiliTask3_GotoMine : MiliTask3_Worker {
  virtual MiliTask3* run() {
    BEGIN_COROUTINE
      while (!worker.atMine()) {
        worker.moveMine();
        mili_yield(next_frame3);
      }
    END_COROUTINE(end_coro3);
  }
  MiliTask3_GotoMine(Worker& p_worker, MiliTask3* p_caller)
    :MiliTask3_Worker(p_worker)
  {
    caller = p_caller;
  }
};

struct MiliTask3_Gather : MiliTask3_Worker {
  virtual MiliTask3* run() {
    BEGIN_COROUTINE
      do {
        worker.gather();
        mili_yield(next_frame3);
      } while (worker.isMining());
    END_COROUTINE(end_coro3);
  }
  MiliTask3_Gather(Worker& p_worker, MiliTask3* p_caller)
    :MiliTask3_Worker(p_worker)
  {
    caller = p_caller;
  }
};

struct MiliTask3_Dropoff : MiliTask3_Worker {
  virtual MiliTask3* run() {
    BEGIN_COROUTINE
      while (!worker.atHome()) {
        worker.moveHome();
        mili_yield(next_frame3);
      }
      worker.dropoff();
      END_COROUTINE(end_coro3);
  }
  MiliTask3_Dropoff(Worker& p_worker, MiliTask3* p_caller)
    :MiliTask3_Worker(p_worker)
  {
    caller = p_caller;
  }
};

struct MiliTask3Main : MiliTask3_Worker {
  virtual MiliTask3* run() {
    BEGIN_COROUTINE
      while (true) {
        mili_yield(new MiliTask3_GotoMine(worker, this));
        mili_yield(new MiliTask3_Gather(worker, this));
        mili_yield(new MiliTask3_Dropoff(worker, this));
      }
    END_COROUTINE(end_coro3);
  }
  MiliTask3Main(Worker& p_worker) :MiliTask3_Worker(p_worker) {}
};

struct MiliTask3Mgr : Task {
  queue<MiliTask3*> q;
  int run() {
    do {
      auto curr = q.front();
      q.pop();
      auto awt = curr->run();
      if (awt == next_frame3) {
        q.push(curr);
        break;
      }
      if (awt == end_coro3 && curr->caller != nullptr) {
        q.push(curr->caller);
        continue;
      }
      if (awt->caller != nullptr) {
        q.push(awt);
        continue;
      }
    } while (q.size() > 0);
    return 0;
  }
  MiliTask3Mgr()
    : Task()
  {
    auto t = new MiliTask3Main(worker);
    q.push(t);
  }

  void print(ostream& stream) const {
    stream << "Mili Coroutine3: " << worker.total << endl;
  }
};


inline int runMili3() {
  cout << "MiLi coroutines with await" << endl;
  next_frame3 = new MiliTask3();
  end_coro3 = new MiliTask3();
  auto *task = new MiliTask3Mgr();
  World world(task);
  return world.run();
}