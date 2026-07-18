#pragma once

#include <cmath>
#include <eigen3/Eigen/Core>
#include <eigen3/Eigen/Dense>
#include <map>
#include <string>

#define MODEL 1
/* MODEL sets the propeller model that is used
 * -1   No model at all (i.e. None)
 * 0    Quadratic Thrust and Torque Fit
 * 1    BEM Non-Linear Model
 *
 * CHORD sets the chord length calculation - MODEL must be 2
 * 0    Constant chord length
 * 1    Linear chord length: c(r) = c_outer - r/R * (delta_c)
 *
 * POLAR sets the lift/drag polar calculation - MODEL must be 2
 * 0    Linear Approximation c_l = as * alpha, c_d = const
 * 1    Simple sin(alpha)*cos(alpha) approximation
 * 2    Full stall model (D'Andrea Paper)
 *
 * DIST sets the use of a thrust distortion factor - MODEL must be 2
 * 0    off, i.e. K = 0 for all velocities
 * 1    K = K(v_horizontal)
 */
#if MODEL == 0 or MODEL == -1
#define CHORD 0  // don't change
#define POLAR 0  // don't change
#define DIST 0   // don't change
#elif MODEL == 1
#define CHORD 1
#define POLAR 1
#define DIST 0
#else
#endif

inline constexpr double toRad(const double angle_deg) {
  return angle_deg * M_PI / 180;
}
inline constexpr double toDeg(const double angle_deg) {
  return angle_deg / M_PI * 180;
}
inline Eigen::Vector3d flu2frd(const Eigen::Vector3d flu) {
  return Eigen::Vector3d{flu[0], -flu[1], -flu[2]};
}
inline Eigen::Quaterniond flu2frd(const Eigen::Quaterniond flu) {
  return Eigen::Quaterniond{flu.w(), flu.x(), -flu.y(), -flu.z()};
}
inline Eigen::Vector3d frd2flu(const Eigen::Vector3d frd) {
  return Eigen::Vector3d{frd[0], -frd[1], -frd[2]};
}
inline Eigen::Quaterniond frd2flu(const Eigen::Quaterniond flu) {
  return Eigen::Quaterniond{flu.w(), flu.x(), -flu.y(), -flu.z()};
}

/* Struct to store all parameters for the propeller
 * A static constexpr is not used for all cases because a constexpr
 * variable can't be used for fitting the value.  */
struct Propeller_s {
  /* PARAMETER DECLARATIONS */
  static constexpr double g = 9.81;        // Acceleration [m/s^2]
  static constexpr double ef = 0.1;        // Hinge offset (relative)
  static constexpr double sigma = 0.215;   // Propeller Solidity Ratio
  static constexpr double c = 1.3 * 1e-02; // Chord length [m]
  static constexpr double mb = 1.22e-3;    // Mass of one blade [kg]
  static constexpr double cg = 2.7e-3;     // Distance of prop CoG to shaft
  static constexpr double Mb = mb * cg * g;  // Moment of blade around the hinge

  double rho = 1.204;                 // ISA air density [kg / m^3]
  double R = 5.1 * 2.54 / 2 * 1e-02;  // Propeller Radius in [m]
  double b = 3;                       // Number of Propeller Blades
  double A = M_PI * R * R;            // Propeller Area [m^2] (recomputed in load)
  double e = ef * R;                  // Hinge offset [m]
  double Ib = mb * cg * cg + 0.25 * R * R / 12;  // UNUSED - Inertia of one blade

  double theta0 = toRad(21.77);  // Propeller Pitch [rad]
  double theta1 = toRad(-11);    // Twist over full span [rad], applied as *r/R
  double ci = 1.7 * 1e-02;       // Chord length inside [m]
  double co = 0.7 * 1e-02;       // Chord length outside [m]
  double cl_offset = 0;          // Lift polar offset (agilicious: 0.07)
  double hforce_scale = 1;       // H-force scale (agilicious: 3.0)

#if MODEL == -1
  double cd = 0;
  double cl = 0;
#elif MODEL == 0
  double cd = 1.908873e-08;  // Quadratic fit torque coefficient
  double cl = 1.562522e-06;  // Quadratic fit thrust coefficient
#elif MODEL == 1 && POLAR == 0 && CHORD == 0
  double cd = 7.5923e-02;  // Constant drag coefficient
  double cl = 6.4398;      // Slope of the lift curve in [1/rad]
#elif MODEL == 1 && POLAR == 0 && CHORD == 1
  double cd = 8.6318e-02;  // Constant drag coefficient
  double cl = 7.429395;    // Slope of the lift curve in [1/rad]
#elif MODEL == 1 && POLAR == 1 && CHORD == 0
  double cd = 9.704894;  // Constant drag coefficient
  double cl = 10.04214;  // Slope of the lift curve in [1/rad]
#elif MODEL == 1 && POLAR == 1 && CHORD == 1
  double cd = 13.54894;  // Constant drag coefficient
  double cl = 15.24214;  // Slope of the lift curve in [1/rad]
#elif MODEL == 1 && POLAR == 2 && CHORD == 1
  double cd = 5.223523;       // Propeller Drag Coefficient
  double cl1 = 8.529715;      // Pre-Stall Lift slope
  double cl2 = 1.357441;      // Post-Stall Lift coefficient
  double M = 20;              // Sigmoid-Function Slope
  double alpha0 = toRad(12);  // Stall Angle
#endif
  double k = 5.89;  // Spring constant of the hinge spring

  /* Keys follow the agilicious BEMParameters yaml; pitch/twist in degrees.
   * The config must contain all keys. */
  void load(const std::map<std::string, double>& c) {
#if !(MODEL == 1 && POLAR == 2)
    cl = c.at("lift_coefficient");
#endif
    cd = c.at("drag_coefficient");
    k = c.at("hinge_spring_constant");
    theta0 = toRad(c.at("pitch"));
    theta1 = toRad(c.at("twist"));
    ci = c.at("chord_inner");
    co = c.at("chord_outer");
    cl_offset = c.at("lift_offset");
    hforce_scale = c.at("hforce_scale");
    rho = c.at("air_density");
    R = c.at("radius");
    b = c.at("num_blades");
    A = M_PI * R * R;
  }
};

struct Motor_s {
  static constexpr double tau = 0.033;           // Time Constant of Motor
  static constexpr double inertia = 9.3575e-06;  // Inertia of Motor + Propeller
};

struct Quadcopter_s {
  double mass = 0.752;   // Mass of the vehicle in kg
  double Ixx = 0.00254;  // Mass of the vehicle in kg
  double Iyy = 0.00214;  // Mass of the vehicle in kg
  double Izz = 0.00436;  // Mass of the vehicle in kg
  double dx = 0.078;     // Propeller x offset from CoG
  double dy = 0.1;       // Propeller y offset from CoG
  double dz = 0.027;     // Propeller z offset from CoG

  double thrust_scale = 1;        // Body-z rotor force scale (agilicious: ~1.3)
  double cxy = 1;                 // Drag coefficient horizontal flight
  double cz = 1;                  // Drag coefficient vertical flight
  double Ax = 0.06 * 0.09;        // Area of the Quadcopter
  double Ay = 0.1 * 0.09;         // Area of the Quadcopter
  double Az = 0.1 * 0.06;  // Area of the Quadcopter
  double rho = 1.204;      // Air density

  /* Drag keys follow the agilicious BodyDragParameters yaml */
  void load(const std::map<std::string, double>& c) {
    rho = c.at("air_density");
    thrust_scale = c.at("thrust_scale");
    dx = c.at("dx");
    dy = c.at("dy");
    dz = c.at("dz");
    cxy = c.at("horizontal_drag_coefficient");
    cz = c.at("vertical_drag_coefficient");
    Ax = c.at("frontarea_x");
    Ay = c.at("frontarea_y");
    Az = c.at("frontarea_z");
  }
};
