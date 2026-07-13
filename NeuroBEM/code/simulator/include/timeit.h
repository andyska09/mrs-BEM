#pragma once

#include <stdio.h>

#include <chrono>

/* Simpler version of the runtime struct that is meant to mimic the
 * MATLAB tic() -> toc() functionality */
struct Timer_s {
  std::chrono::steady_clock::time_point t0, t1;
  double dur;

  void start() { t0 = std::chrono::steady_clock::now(); }

  void stop() {
    t1 = std::chrono::steady_clock::now();
    dur = std::chrono::duration<double, std::micro>(t1 - t0).count();
  }

  void print(const char* message = "Elapsed Time") {
    printf("%s: %f micro seconds\n", message, dur);
  }
};
