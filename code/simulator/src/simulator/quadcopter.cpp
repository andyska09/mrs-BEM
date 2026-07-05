#include "quadcopter.h"

/* To use OpenMP on Eigen::Vector3d Types one needs to define a custom
 * reduction operation. Otherwise OpenMP can't be used with Eigen */
#pragma omp declare reduction(+:Eigen::Vector3d: omp_out=omp_out+omp_in) \
                initializer(omp_priv=Eigen::Vector3d::Zero())

/* Constructor of the Quadcopter Class. Currently creates a drone with 4
 * propellers */
Quadcopter::Quadcopter() {
  motors.reserve(4);
  motors.emplace_back(
      Eigen::Vector3d(
          std::vector<double>{-param.dx, param.dy, param.dz}.data()),
      true);
  motors.emplace_back(
      Eigen::Vector3d(std::vector<double>{param.dx, param.dy, param.dz}.data()),
      false);
  motors.emplace_back(
      Eigen::Vector3d(
          std::vector<double>{-param.dx, -param.dy, param.dz}.data()),
      false);
  motors.emplace_back(
      Eigen::Vector3d(
          std::vector<double>{param.dx, -param.dy, param.dz}.data()),
      true);
#ifdef _OPENMP
  omp_set_num_threads(motors.size());
#endif
}

Quadcopter::~Quadcopter() {}

/* Internal function to update the state of the class */
void Quadcopter::_update() {
  _calculateThrust();
  _calculateTorque();
  _calculateDrag();
  _valid = true;
}

Eigen::Vector3d Quadcopter::getThrust() {
  if (!_valid) _update();
  return thrust;
}

Eigen::Vector3d Quadcopter::getTorque() {
  if (!_valid) _update();
  return torque;
}

Eigen::Vector3d Quadcopter::getDrag() {
  if (!_valid) _update();
  return drag;
}

Eigen::Vector3d Quadcopter::getForce() {
  if (!_valid) _update();
  return drag + thrust;
}

std::vector<double> Quadcopter::getDiagnostics() {
  if (!_valid) _update();
  std::vector<double> out;
  for (Motor& m : motors) {
    Eigen::Vector3d d = m.getDiagnostics();
    out.push_back(d[0]);
    out.push_back(d[1]);
    out.push_back(d[2]);
  }
  return out;
}

void Quadcopter::setAero(double cl, double cd, double k) {
  for (Motor& m : motors) m.setAero(cl, cd, k);
  _valid = false;
}

bool Quadcopter::setAcceleration(const Eigen::Vector3d acc) {
  this->acc = acc;
  _valid = false;
  return true;
}

bool Quadcopter::setVelocity(const Eigen::Vector3d vel) {
  this->vel = vel;
  _valid = false;
  for (Motor& mot : motors) {
    mot.setBodyVelocity(this->vel);
  }
  return true;
}

bool Quadcopter::setPosition(const Eigen::Vector3d pos) {
  this->pos = pos;
  _valid = false;
  return true;
}

bool Quadcopter::setAngularAcceleration(const Eigen::Vector3d angAcc) {
  this->angAcc = angAcc;
  _valid = false;
  return true;
}

bool Quadcopter::setAngularVelocity(const Eigen::Vector3d angVel) {
  this->angVel = angVel;
  _valid = false;
  for (Motor& mot : motors) {
    mot.setAttitudeRate(angVel);
  }
  return true;
}

bool Quadcopter::setAttitude(const Eigen::Vector3d att) {
  this->att = att;
  _valid = false;
  return true;
}

bool Quadcopter::setQuaternionAttitude(const Eigen::Quaterniond quat) {
  this->qt = quat;
  _valid = false;
  return true;
}

bool Quadcopter::setMotorOmega(const std::vector<double> omegas) {
  if (omegas.size() != motors.size()) {
    printf("Quadcopter::setMotorOmega wrong number of omegas supplied\n");
    return false;
  }

  for (size_t i = 0; i < motors.size(); ++i) motors[i].setMotorOmega(omegas[i]);

  _valid = false;
  return true;
}

bool Quadcopter::setMotorAcceleration(const std::vector<double> domegas) {
  if (domegas.size() != motors.size()) {
    printf(
        "Quadcopter::setMotorAcceleration wrong number of omegas supplied\n");
    return false;
  }

  for (size_t i = 0; i < motors.size(); ++i)
    motors[i].setMotorAcceleration(domegas[i]);

  _valid = false;
  return true;
}

/* The thrust from the vehicle's individual propellers is summed up in
 * parallel using an OpenMP reduction */
bool Quadcopter::_calculateThrust() {
  thrust.setZero();
#pragma omp parallel for reduction(+ : thrust)
  for (size_t i = 0; i < motors.size(); ++i) thrust += motors[i].getThrust();
  return true;
}

/* The torque from the vehicle's individual propellers is summed up in
 * parallel using an OpenMP reduction */
bool Quadcopter::_calculateTorque() {
  torque.setZero();
#pragma omp parallel for reduction(+ : torque)
  for (size_t i = 0; i < motors.size(); ++i) {
    torque += motors[i].getTorque();
    torque += motors[i].getOffset().cross(motors[i].getThrust());
  }
  return true;
}

/* To calculate the drag of the vehicle a uniform drag coefficient is assumed
 * in the x-y plane (but with varying surface area).  */
bool Quadcopter::_calculateDrag() {
  drag = -0.5 * param.rho * vel * vel.norm();
  drag[0] *= param.cxy * param.Ax;
  drag[1] *= param.cxy * param.Ay;
  drag[2] *= param.cz * param.Az;
  return true;
}

/* Function to print detailed information on the vehicles state. If
 * a true boolean is passed as an argument, the print will also call
 * the prints from the subroutines. */
void Quadcopter::printInfo(const bool detailed) {
  if (!_valid) _update();
  printf("\n######## Quadcopter Information ##########\n");
  printf("%-30s%9s   %9s   %9s\n", " ", "x", "y", "z");
  printf("%-30s% 9.4f   % 9.4f   % 9.4f\n", "Vehicle Position [m]", pos[0],
         pos[1], pos[2]);
  printf("%-30s% 9.4f   % 9.4f   % 9.4f\n", "Body Velocity [m/s]", vel[0],
         vel[1], vel[2]);
  printf("%-30s% 9.4f   % 9.4f   % 9.4f\n", "Body Acceleration [m/s^2]", acc[0],
         acc[1], acc[2]);
  printf("%-30s% 9.4f   % 9.4f   % 9.4f\n", "Euler Angles [deg]", toDeg(att[0]),
         toDeg(att[1]), toDeg(att[2]));
  printf("%-30s% 9.4f   % 9.4f   % 9.4f\n", "Angular Body Rates [deg/s]",
         toDeg(angVel[0]), toDeg(angVel[1]), toDeg(angVel[2]));
  printf("%-30s% 9.4f   % 9.4f   % 9.4f\n", "Angular Body Acc [deg/s^2]",
         toDeg(angAcc[0]), toDeg(angAcc[1]), toDeg(angAcc[2]));
  printf("%-30s% 9.4f   % 9.4f   % 9.4f\n", "Propeller Thrust [N]", thrust[0],
         thrust[1], thrust[2]);
  printf("%-30s% 9.4f   % 9.4f   % 9.4f\n", "Propeller Torque [Nm]", torque[0],
         torque[1], torque[2]);
  printf("%-30s% 9.4f   % 9.4f   % 9.4f\n", "Body Drag [N]", drag[0], drag[1],
         drag[2]);

  if (detailed)
    for (size_t i = 0; i < motors.size(); ++i) motors[i].printInfo(i + 1);
}
