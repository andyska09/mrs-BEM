#pragma once

#include <eigen3/Eigen/Core>
#include <eigen3/Eigen/Dense>
#include <vector>

#include "motor.h"
#include "omp.h"
#include "timeit.h"

/* Class that implements a Quadcopter. All units are in SI units and the
 * coordinate system is front-right-down as customary in helicopter literature
 */
class Quadcopter {
 public:
  Quadcopter();
  ~Quadcopter();
  bool setAcceleration(const Eigen::Vector3d acc);
  bool setVelocity(const Eigen::Vector3d vel);
  bool setPosition(const Eigen::Vector3d pos);
  bool setAngularAcceleration(const Eigen::Vector3d angAcc);
  bool setAngularVelocity(const Eigen::Vector3d angVel);
  bool setAttitude(const Eigen::Vector3d att);
  bool setQuaternionAttitude(const Eigen::Quaterniond quat);
  bool setMotorOmega(const std::vector<double> omegas);
  bool setMotorAcceleration(const std::vector<double> domegas);
  Eigen::Vector3d getThrust();
  Eigen::Vector3d getTorque();
  Eigen::Vector3d getDrag();
  Eigen::Vector3d getForce();
  void printInfo(const bool detailed = false);

 private:
  bool _valid = false;
  std::vector<Motor> motors;
  Eigen::Vector3d acc = {0, 0, 0};
  Eigen::Vector3d vel = {0, 0, 0};
  Eigen::Vector3d pos = {0, 0, 0};
  Eigen::Vector3d angAcc = {0, 0, 0};
  Eigen::Vector3d angVel = {0, 0, 0};
  Eigen::Vector3d att = {0, 0, 0};
  Eigen::Quaterniond qt = {1, 0, 0, 0};
  Eigen::Vector3d thrust = {0, 0, 0};
  Eigen::Vector3d torque = {0, 0, 0};
  Eigen::Vector3d drag = {0, 0, 0};
  static constexpr Quadcopter_s param = Quadcopter_s();
  Timer_s timer;

  void _update();
  bool _calculateThrust();
  bool _calculateTorque();
  bool _calculateDrag();
};
