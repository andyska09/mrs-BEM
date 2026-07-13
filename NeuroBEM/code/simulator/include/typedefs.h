#pragma once

#include <stdio.h>

#include <chrono>
#include <cmath>

/* Struct to facilitate measuring the runtime of any code segment in between
 * start() and stop() */
struct Runtime_s {
 private:
  double sum = 0;
  double sum_sq = 0;
  size_t num = 0;
  std::chrono::time_point<std::chrono::steady_clock> t0;
  std::chrono::time_point<std::chrono::steady_clock> t1;

  bool add() {
    double dt = std::chrono::duration<double, std::micro>(t1 - t0).count();
    ++num;
    sum += dt;
    sum_sq += dt * dt;
    return true;
  };

 public:
  void start() { t0 = std::chrono::steady_clock::now(); }
  void stop() {
    t1 = std::chrono::steady_clock::now();
    add();
  }
  double mean() { return sum / num; }
  double std() { return std::sqrt(sum_sq / num - sum * sum / (num * num)); }
  void print(const char* message = "Runtime") {
    printf("%-30s% 5.1f +- %5.2f\n", message, mean(), std());
  }
};

