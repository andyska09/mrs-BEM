#include "propeller.h"

#define isCW() (isCW ? 1 : -1)

/* Constructor for the propeller class */
Propeller::Propeller(const bool isCW) { this->isCW = isCW; }

/* Destructor for the propeller class */
Propeller::~Propeller() { delete solver; }

/* Calculates the thrust based on blade element theory. The numerical
 *  integration is performed through the GSL library */
double Propeller::_calculateThrust() {
  double v1 = _calculateInducedVelocity();
  thrustRT.start();
#if MODEL == -1
  double T = 0;
#elif MODEL == 0
  double T = param.cl * Omega * Omega;
#elif MODEL == 1
  solver->setPropellerState(Omega, vtot, vel[2], K, alpha, mu, v1);
  double T = solver->integrateThrust();
#endif
  thrustRT.stop();
  return T;
};

/* Calculates the drag torque based on blade element theory. The numerical
 * integration is performed through the GSL library */
double Propeller::_calculateTorque() {
  double v1 = _calculateInducedVelocity();
  torqueRT.start();
#if MODEL == -1
  double Q = 0;
#elif MODEL == 0
  double Q = param.cd * Omega * Omega;
#elif MODEL == 1
  solver->setPropellerState(Omega, vtot, vel[2], K, alpha, mu, v1);
  double Q = solver->integrateTorque();
#endif
  torqueRT.stop();
  return Q;
};

/* Calculates the horizontal force of the propeller using blade element theory
 * The numerical integration is performed through the GSL library */
double Propeller::_calculateHForce() {
  double v1 = _calculateInducedVelocity();
  hforceRT.start();
#if MODEL == -1
  double H = 0;
#elif MODEL == 0
  double H = 0;
#elif MODEL == 1
  solver->setPropellerState(Omega, vtot, vel[2], K, alpha, mu, v1);
  double H = 3.0 * solver->integrateHForce();
#endif
  hforceRT.stop();
  return H;
}

/* Calculates the induced velocity given omega. The numerical equation solving
 * is done through the GSL library */
double Propeller::_calculateInducedVelocity() {
  velocityRT.start();
#if MODEL == 0 or MODEL == -1
  v1 = 0;
#elif MODEL == 1
  if (!_validv1) {
    solver->setPropellerState(Omega, vtot, vel[2], K, alpha, mu);
    v1 = solver->solveInducedVelocity();
  }
#endif
  velocityRT.stop();
  _validv1 = true;
  return v1;
}

/* Calculates the Advance Ratio of a Propeller */
double Propeller::_calculateAdvanceRatio() {
  if (std::fabs(Omega) < 1)
    return 0;
  else
    return vhor / (Omega * param.R);
}

/* Calculates the angle of attack of the shaft */
double Propeller::_calculateShaftAngleOfAttack() {
  if ((std::fabs(vel[2]) < 1E-6))
    return 0;
  else
    return std::atan2(vel[2], vhor);
}

/* Calculates the induced velocity that is required for a certain thrust */
double Propeller::_calculateInducedVelocity(const double thrust) {
  return std::sqrt(thrust / (2 * param.rho * param.A));
}

/* Updates the state dependent internal parameters to according to the new
 * state. This function must be called before any getter method */
void Propeller::_update() {
  vhor = std::sqrt(vel[0] * vel[0] + vel[1] * vel[1]) + 1E-6;
  vtot = std::sqrt(vhor * vhor + vel[2] * vel[2]) + 1E-6;
#if DIST == 1
  K = std::fmin(0.25 * vhor, 1);
#else
  K = 0;
#endif
  alpha = _calculateShaftAngleOfAttack();
  mu = _calculateAdvanceRatio();
  vind = _calculateInducedVelocity();
  thrust = _calculateThrust();
  torque = _calculateTorque();
  hforce = _calculateHForce();

#if MODEL == 0
  a0 = 0;
  a1s = 0;
  b1s = 0;
#else
  a0 = _calculateConing();
  a1s = _calculateLongitudinalFlapping();
  b1s = _calculateLateralFlapping();
#endif

  // the x-axis of BEM opposes the wind and the isCW macro takes care of
  // CW or CCW propellers
  Eigen::Matrix3d rot_mat =
      Eigen::AngleAxisd{std::atan2(vel[1], vel[0]), Eigen::Vector3d::UnitZ()}
          .toRotationMatrix();
  forceVec = rot_mat * Eigen::Vector3d{-(hforce + std::sin(a1s) * thrust),
                                       isCW() * std::sin(b1s) * thrust,
                                       -thrust * cos(a0)};
  torqueVec = rot_mat * Eigen::Vector3d{isCW() * param.k * b1s, param.k * a1s,
                                        -isCW() * torque};
  _valid = true;
}

/* Update the internal attitude rate state of the propeller */
bool Propeller::setAttitudeRate(const Eigen::Vector3d att_rate) {
  this->rate = att_rate;
  _valid = false;
  _validv1 = false;
  return true;
}

/* Update the internal free stream velocity state of the propeller */
bool Propeller::setVelocity(const Eigen::Vector3d velocity) {
  this->vel = velocity;
  _valid = false;
  _validv1 = false;
  return true;
}

/* Update the internal free stream velocity state of the propeller */
bool Propeller::setOmega(const double Omega) {
  this->Omega = Omega;
  _valid = false;
  _validv1 = false;
  return true;
}

/* Update the internal state of the propeller */
bool Propeller::setState(const double Omega, const Eigen::Vector3d velocity,
                         const Eigen::Vector3d att_rate) {
  setAttitudeRate(att_rate);
  setVelocity(velocity);
  setOmega(Omega);
  _valid = false;
  _validv1 = false;
  return true;
}

double Propeller::getThrust() {
  if (!_valid) _update();
  return thrust;
}

double Propeller::getTorque() {
  if (!_valid) _update();
  return torque;
}

double Propeller::getHForce() {
  if (!_valid) _update();
  return hforce;
}

Eigen::Vector3d Propeller::getForceVector() {
  if (!_valid) _update();
  return forceVec;
}

Eigen::Vector3d Propeller::getTorqueVector() {
  if (!_valid) _update();
  return torqueVec;
}

double Propeller::getConing() {
  if (!_valid) _update();
  return a0;
}

double Propeller::getLateralFlapping() {
  if (!_valid) _update();
  return b1s;
}

double Propeller::getLongitudinalFlapping() {
  if (!_valid) _update();
  return a1s;
}

Eigen::Vector3d Propeller::getDiagnostics() {
  if (!_valid) _update();
  return Eigen::Vector3d{vind, mu, alpha};
}

void Propeller::setAero(double cl, double cd, double k) {
  param.cl = cl;
  param.cd = cd;
  param.k = k;
  _valid = false;
  _validv1 = false;
}

/* Summarize the current propeller data and runtime information on the command
 * line */
void Propeller::printInfo() {
  if (!_valid) _update();
  printf("\n######## Propeller Information ##########\n");
  printf("%-30s%9s\n", "Propeller Direction", (isCW ? "CW" : "CCW"));
  printf("%-30s% 9.1f\n", "Propeller Speed [rad/s]", Omega);
  printf("%-30s% 9.4f   % 9.4f   % 9.4f\n", "Free Stream velocity [m/s]",
         vel[0], vel[1], vel[2]);
  printf("%-30s% 9.4f   % 9.4f   % 9.4f\n", "Attitude Rates [deg/s]",
         toDeg(rate[0]), toDeg(rate[1]), toDeg(rate[2]));
  printf("%-30s% 9.4f\n", "Advance Ratio", mu);
  printf("%-30s% 9.4f\n", "Shaft Angle of Attack [deg]", toDeg(alpha));
  printf("%-30s% 9.4f   % 9.4f   % 9.4f\n", "Propeller Thrust", forceVec[0],
         forceVec[1], forceVec[2]);
  printf("%-30s% 9.4f   % 9.4f   % 9.4f\n", "Propeller Torque", torqueVec[0],
         torqueVec[1], torqueVec[2]);
  printf("%-30s% 9.4f\n", "Induced Velocity [m/s]", vind);
  printf("%-30s% 9.4f\n", "Coning Angle a0 [deg]", toDeg(a0));
  printf("%-30s% 9.4f\n", "Longitudinal Angle a1s [deg]", toDeg(a1s));
  printf("%-30s% 9.4f\n", "Lateral Angle b1s [deg]", toDeg(b1s));
  printf("\n######## Propeller Performance Information ##########\n");
  printf("%-30s% 5.1f +- %5.2f\n", "Runtimes Thrust [µs]", thrustRT.mean(),
         thrustRT.std());
  printf("%-30s% 5.1f +- %5.2f\n", "Runtimes Torque [µs]", torqueRT.mean(),
         torqueRT.std());
  printf("%-30s% 5.1f +- %5.2f\n", "Runtimes HForce [µs]", hforceRT.mean(),
         hforceRT.std());
  printf("%-30s% 5.1f +- %5.2f\n", "Runtimes Velocity [µs]", velocityRT.mean(),
         velocityRT.std());
  printf("Thrust: %f\tTorque %f\tHForce %f\n", thrust, torque, hforce);
}
