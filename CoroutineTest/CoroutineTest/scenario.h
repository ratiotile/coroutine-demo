#pragma once

#include <vector>
using namespace std;

const int distance_to_mine = 10;
const int worker_speed = 1;
const int worker_capacity = 1;
const int mining_frames = 10;
const int num_tasks = 1000;
const int frames_to_run = 1000000;


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
