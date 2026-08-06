#pragma once

#include <cmath>
#include <Eigen/Core>
#include <Eigen/Dense>
#include <map>
#include <string>
#include <vector>

namespace mybem {

/* Numeric parameters: everything CMA-ES can move. */
using Params = std::map<std::string, double>;
/* Structural choices: polar form, chord law, on/off switches. Never tuned. */
using Options = std::map<std::string, std::string>;

inline constexpr double toRad(double deg) { return deg * M_PI / 180.0; }
inline constexpr double toDeg(double rad) { return rad / M_PI * 180.0; }

inline Eigen::Vector3d flu2frd(const Eigen::Vector3d& v) {
  return {v[0], -v[1], -v[2]};
}
inline Eigen::Vector3d frd2flu(const Eigen::Vector3d& v) {
  return {v[0], -v[1], -v[2]};
}
inline Eigen::Quaterniond flu2frd(const Eigen::Quaterniond& q) {
  return {q.w(), q.x(), -q.y(), -q.z()};
}

struct Wrench {
  Eigen::Vector3d force = Eigen::Vector3d::Zero();
  Eigen::Vector3d torque = Eigen::Vector3d::Zero();

  Wrench& operator+=(const Wrench& o) {
    force += o.force;
    torque += o.torque;
    return *this;
  }
};

/* One row of flight data, body frame FRD. */
struct State {
  Eigen::Vector3d vel = Eigen::Vector3d::Zero();
  Eigen::Vector3d omega = Eigen::Vector3d::Zero();
  std::vector<double> mot;
  std::vector<double> dmot;
};

struct TunableParam {
  std::string key;
  double def, lo, hi;
};

/* Rotor placement and the scale on summed propeller thrust. Not a component:
 * it is what the components act on. */
struct Airframe {
  double dx = 0.078;
  double dy = 0.100;
  double dz = 0.027;
  double thrust_scale = 1.0;

  /* Dataset order: back-right, front-right, back-left, front-left. */
  std::vector<Eigen::Vector3d> offsets() const {
    return {{-dx, dy, dz}, {dx, dy, dz}, {-dx, -dy, dz}, {dx, -dy, dz}};
  }
  static const std::vector<bool>& spinCW() {
    static const std::vector<bool> cw = {true, false, false, true};
    return cw;
  }

  std::vector<TunableParam> tunables() const {
    return {{"thrust_scale", 1.0, 0.5, 2.0},
            {"dx", 0.078, 0.04, 0.15},
            {"dy", 0.100, 0.05, 0.20},
            {"dz", 0.027, -0.05, 0.10}};
  }

  void load(const Params& p) {
    thrust_scale = p.at("thrust_scale");
    dx = p.at("dx");
    dy = p.at("dy");
    dz = p.at("dz");
  }

  Params params() const {
    return {{"thrust_scale", thrust_scale}, {"dx", dx}, {"dy", dy}, {"dz", dz}};
  }
};

}  // namespace mybem
