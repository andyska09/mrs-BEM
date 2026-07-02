#pragma once

#include <stdio.h>
#include <stdlib.h>

#include <chrono>
#include <cmath>
#include <eigen3/Eigen/Core>
#include <vector>

#include "gslHelper.h"
#include "params.h"
#include "typedefs.h"

/* Class that implements a propeller model. Depending on the defines, it either
 * uses BEM to compute the acting forces and torques or a simple quadratic fit
 * instead. All quantities are in their corresponding SI-units. The frame is
 * centered at the propeller mount with the x-axis being aligned with the front
 * direction of the vehicle. Note that a front-right-down coordinate system is
 * used.
 */
class Propeller {
 public:
  Propeller(const bool isCW = true);
  ~Propeller();

  /* FUNCTION DECLARATIONS */
  Eigen::Vector3d getForceVector();
  Eigen::Vector3d getTorqueVector();
  double getThrust();
  double getTorque();
  double getHForce();
  double getConing();
  double getLongitudinalFlapping();
  double getLateralFlapping();
  bool setState(const double Omega, const Eigen::Vector3d velocity,
                const Eigen::Vector3d att_rate);
  bool setOmega(const double Omega);
  bool setVelocity(const Eigen::Vector3d velocity);
  bool setAttitudeRate(const Eigen::Vector3d att_rate);

  void printInfo();

 private:
  /* FUNCTION DECLARATIONS */
  double _calculateThrust();
  double _calculateTorque();
  double _calculateHForce();
  double _calculateInducedVelocity();
  double _calculateInducedVelocity(const double thrust);
  double _calculateAdvanceRatio();
  double _calculateShaftAngleOfAttack();
  double _calculateConing();
  double _calculateLateralFlapping();
  double _calculateLongitudinalFlapping();
  void _update();

  /* VARIABLE DECLARATIONS */
  double K = 0;       // Thrust distortion factor
  double Omega = 0;   // Speed of the propeller
  double mu = 0;      // Advance Ratio
  double alpha = 0;   // Angle of attack of shaft-normal
  double vhor = 0;    // xy-plane velocity of propeller
  double vtot = 0;    // norm of the free stream velocity
  double vind = 0;    // induced velocity
  double thrust = 0;  // thrust of the propeller
  double torque = 0;  // drag torque of the propeller
  double hforce = 0;  // h-force of the propeller
  double a0 = 0;      // coning angle
  double a1s = 0;     // longitudinal flapping angle
  double b1s = 0;     // lateral flapping angle
  bool isCW = true;
  bool _valid = false;  // bool that is false if any state data changed
  bool _validv1 = false;
  double v1 = 0;

  Eigen::Vector3d vel = {0, 0, 0};   // propeller velocity in local motor frame
  Eigen::Vector3d rate = {0, 0, 0};  // Attitude rate of the propeller
  Eigen::Vector3d forceVec = {0, 0, 0};
  Eigen::Vector3d torqueVec = {0, 0, 0};

  static constexpr Propeller_s param = Propeller_s();
  Runtime_s thrustRT, torqueRT, hforceRT, velocityRT;
  GSLHelper* solver = new GSLHelper();
};

