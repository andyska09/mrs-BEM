#pragma once

#include <stdio.h>

#include <cmath>
#include <eigen3/Eigen/Core>
#include <eigen3/Eigen/Dense>
#include <vector>

#include "propeller.h"

class Motor {
 public:
  Motor(const Eigen::Vector3d offset, const bool isCW = true);
  ~Motor();
  bool setBodyVelocity(const Eigen::Vector3d vel_B);
  bool setAttitudeRate(const Eigen::Vector3d rate_B);
  bool setMotorOmega(const double Omega);
  bool setMotorAcceleration(const double dOmega);
  bool simulateOmega(const double OmegaDesired);
  Eigen::Vector3d getThrust();
  Eigen::Vector3d getTorque();
  Eigen::Vector3d getOffset();
  Eigen::Vector3d getDiagnostics();
  void setAero(double cl, double cd, double k);
  void printInfo(const int num = 0);

 private:
  static constexpr Motor_s param = Motor_s();
  double Omega = 1200;
  double dOmega = 0;
  bool _valid = false;
  bool isCW = false;

  Eigen::Vector3d offset = {0, 0, 0};  // w.r.t. CoG
  Eigen::Vector3d force = {0, 0, 0};   // in body-frame
  Eigen::Vector3d torque = {0, 0, 0};  // in body-frame
  Eigen::Vector3d vel_B = {0, 0, 0};   // velocity of the drone vel_B
  Eigen::Vector3d vel_M = {0, 0, 0};   // velocity in motor frame vel_M
  Eigen::Vector3d rate_B = {0, 0, 0};  // angular rate of the vehicle

  Propeller p;

  bool _update();
};
