#pragma once

#include "mybem/types.h"

namespace mybem {

/* Mass and inertia: the only place they are defined. Everything that turns a
 * measured acceleration into a force or torque reads them from here. */
struct Drone {
  std::string name;
  double mass = 0;
  Eigen::Vector3d inertia = Eigen::Vector3d::Zero();

  static Drone load(const std::string& path);

  Wrench measured(const Eigen::Vector3d& acc, const Eigen::Vector3d& ang_acc,
                  const Eigen::Vector3d& ang_vel) const {
    return {mass * acc, inertia.cwiseProduct(ang_acc) +
                            ang_vel.cross(inertia.cwiseProduct(ang_vel))};
  }
};

}  // namespace mybem
