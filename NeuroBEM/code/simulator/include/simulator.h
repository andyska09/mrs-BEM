#pragma once

#include <eigen3/Eigen/Core>
#include <eigen3/Eigen/Dense>
#include <iostream>

#include "csvReader.h"
#include "csvWriter.h"
#include "motor.h"
#include "propeller.h"
#include "quadcopter.h"
#include "timeit.h"

class Simulator {
 public:
  Simulator();
  Simulator(const char* infile, const char* outfile, const double* aero = nullptr);
};
