#pragma once
#include <iostream>
#include <chrono>
#include <string>
using namespace std;

const int DISTANCE_TO_MINE = 10;
const int MINING_TIME = 10;
const int MILLISECONDS = 1000;


struct Worker {
  int total = 0;
  int carrying = 0;
  int position = 0; // distance from home
  int mining_progress = 0;
  void gather() {
    mining_progress++;
    if (mining_progress == MINING_TIME) {
      carrying = 8;
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
    position++;
  }
  void moveHome() {
    position--;
  }
  bool atMine() {
    return position == DISTANCE_TO_MINE;
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
};

inline ostream& operator<<(ostream& stream, Task const & task) {
  task.print(stream);
  return stream;
}

struct World {
  int frame = 0;
  Task* task;

  void nextFrame() {
    frame++;
    task->run();
  }

  int run() {
    auto start = chrono::high_resolution_clock::now();
    while (chrono::duration_cast<chrono::milliseconds>(
      chrono::high_resolution_clock::now() - start).count() < MILLISECONDS) 
    {
      nextFrame();
    }
    return end();
  }

  int end() {
    cout << "Time up: " << *task << endl;
    int score = task->worker.total;
    delete task;
    return score;
  }

  World(Task* t) {
    task = t;
  }
};

