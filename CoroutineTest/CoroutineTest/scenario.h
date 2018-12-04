#pragma once
#include <iostream>
#include <chrono>
#include <string>
#include <vector>
using namespace std;

const int distance_to_mine = 10;
const int worker_speed = 1;
const int worker_capacity = 1;
const int mining_frames = 10;
const int num_tasks = 1000;
const int frames_to_run = 10000000;


struct Worker {
  int total = 0;
  int carrying = 0;
  int position = 0; // distance from home
  int mining_progress = 0;
  void gather() {
    mining_progress++;
    if (mining_progress >= mining_frames) {
      carrying = worker_capacity;
      mining_progress = 0;
    }
  }
  bool isMining() {
    return mining_progress > 0;
  }
  void dropoff() {
    total += carrying;
    carrying = 0;
  }
  void moveMine() {
    position += worker_speed;
  }
  void moveHome() {
    position -= worker_speed;
  }
  bool atMine() {
    return position == distance_to_mine;
  }
  bool atHome() {
    return position == 0;
  }
};

struct Task {
  Worker worker;
  virtual int run() = 0;
  virtual void print(ostream& stream) const {
    stream << "overload this" << endl;
  }
  virtual ~Task() = default;
};

inline ostream& operator<<(ostream& stream, Task const & task) {
  task.print(stream);
  return stream;
}

struct TaskManager {
  std::vector<Task*> m_tasks;

  void addTask(Task* t) {
    m_tasks.push_back(t);
  }

  void cleanup() {
    for(auto t : m_tasks) {
      delete t;
    }
    m_tasks.clear();
  }
};

struct World {
  int frame = 0;
  Task* task;

  void nextFrame() {
    frame++;
    task->run();
  }

  int run() {
    auto const start = chrono::high_resolution_clock::now();
    for (int i = 0; i < frames_to_run; ++i) 
    {
      nextFrame();
    }
    auto const endt = chrono::high_resolution_clock::now();
    auto diff = endt - start;
    return end(diff);
  }

  int end(chrono::nanoseconds ns) {
    cout << "Completed trips: " << *task << " Time: " << ns.count() << endl;
    int score = task->worker.total;
    delete task;
    return score;
  }

  World(Task* t) {
    task = t;
  }
};

