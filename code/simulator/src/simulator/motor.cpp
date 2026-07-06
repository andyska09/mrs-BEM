#include "motor.h"

#define isCW() (isCW ? 1 : -1)

/* Constructor for motor class. The offset (from the vehicle CoG) and
 * a boolean indicating whether the motor turn clockwise must be supplied */
Motor::Motor(const Eigen::Vector3d offset, const bool isCW) : p(Propeller(isCW)) {
    this->offset = offset;
    this->isCW = isCW;
}


Motor::~Motor() {

}


/* Private function to update the motor and propeller state */
bool Motor::_update() {
    vel_M = vel_B + rate_B.cross(offset);
    p.setState(Omega, vel_M, rate_B);
    force = p.getForceVector();
    torque = p.getTorqueVector();
    torque[3] -= isCW() * dOmega * param.inertia;
    _valid = true;
    return true;
}


/* Sets the velocity of the motor */
bool Motor::setBodyVelocity(const Eigen::Vector3d vel_B) {
    this->vel_B = vel_B;
    _valid = false;
    p.setVelocity(vel_B);
    return true;
}


/* Sets the attitude rate of the motor in rad/s */
bool Motor::setAttitudeRate(const Eigen::Vector3d rate_B) {
    this->rate_B = rate_B;
    _valid = false;
    return true;
}


/* Sets the angular velocity of the motor in rad/s */
bool Motor::setMotorOmega(const double Omega) {
    this->Omega = Omega;
    _valid = false;
    return true;
};



/* Sets the angular acceleration of the motor in rad * s^-2 */
bool Motor::setMotorAcceleration(const double dOmega) {
    this->dOmega = dOmega;
    _valid = false;
    return true;
}


/* Returns the force vector (including hforce) from the propeller */
Eigen::Vector3d Motor::getThrust() {
    if (!_valid) _update();
    return force;
}


/* Returns the torque vector due to propeller and motor torque */
Eigen::Vector3d Motor::getTorque() {
    if (!_valid) _update();
    return torque;
}


/* Returns the offset vector from the center of gravity to the motor */
Eigen::Vector3d Motor::getOffset() {
    return offset;
}


/* Function to print the motor info. Optionally an integer can be passed to
 * identify which motor information is printed. */
void Motor::printInfo(const int num) {
    if (!_valid) _update();
    if (num != 0)
        printf("\n######## Motor %i Information ##########\n", num);
    else
        printf("\n######## Motor Information ##########\n");
    printf("%-30s%9s   %9s   %9s\n", " ", "x", "y", "z");
    printf("%-30s% 9.4f   % 9.4f   % 9.4f\n", "Body Velocity", vel_B[0], vel_B[1], vel_B[2]);
    printf("%-30s% 9.4f   % 9.4f   % 9.4f\n", "Motor Velocity", vel_M[0], vel_M[1], vel_M[2]);
    printf("%-30s% 9.4f   % 9.4f   % 9.4f\n", "Motor Thrust", force[0], force[1], force[2]);
    printf("%-30s% 9.4f   % 9.4f   % 9.4f\n", "Motor Torque", torque[0], torque[1], torque[2]);
    p.printInfo();
}

