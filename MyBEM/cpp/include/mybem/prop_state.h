#pragma once

#include "mybem/types.h"

namespace mybem {

/* Kinematic state of one propeller, motor frame FRD. The derived fields below
 * are filled by PropellerModel::evaluate. */
struct PropState {
  double Omega = 0;
  Eigen::Vector3d vel = Eigen::Vector3d::Zero();
  Eigen::Vector3d rate = Eigen::Vector3d::Zero();
  bool cw = true;
  size_t index = 0;

  double vhor = 0;
  double vtot = 0;
  double K = 0;
  double alpha = 0;
  double mu = 0;
  double vind = 0;

  int spin() const { return cw ? 1 : -1; }
};

}  // namespace mybem
