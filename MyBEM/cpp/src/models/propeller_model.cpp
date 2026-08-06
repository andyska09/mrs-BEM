#include "mybem/models/propeller_model.h"

#include <cmath>

#include "mybem/bem/flapping.h"

namespace mybem {

void PropellerModel::add(const State& s, const Airframe& af, Wrench& w) {
  const std::vector<Eigen::Vector3d> offsets = af.offsets();
  const std::vector<bool>& cw = Airframe::spinCW();

  Eigen::Vector3d thrust_sum = Eigen::Vector3d::Zero();
  Eigen::Vector3d torque_sum = Eigen::Vector3d::Zero();

  for (size_t i = 0; i < s.mot.size() && i < offsets.size(); ++i) {
    PropState ps;
    ps.index = i;
    ps.cw = cw[i];
    ps.Omega = s.mot[i];
    ps.rate = s.omega;
    ps.vel = s.vel + s.omega.cross(offsets[i]);

    const Wrench pw = evaluate(ps);
    thrust_sum += pw.force;
    torque_sum += pw.torque + offsets[i].cross(pw.force);
  }

  // Force-z only, after the moment arms have used unscaled thrust.
  thrust_sum[2] *= af.thrust_scale;

  w.force += thrust_sum;
  w.torque += torque_sum;
}

Wrench PropellerModel::evaluate(PropState& p) {
  p.vhor = std::sqrt(p.vel[0] * p.vel[0] + p.vel[1] * p.vel[1]) + 1e-6;
  p.vtot = std::sqrt(p.vhor * p.vhor + p.vel[2] * p.vel[2]) + 1e-6;
  p.K = distortion_ ? std::fmin(0.25 * p.vhor, 1) : 0;
  p.alpha =
      std::fabs(p.vel[2]) < 1e-6 ? 0 : std::atan2(p.vel[2], p.vhor);
  p.mu = std::fabs(p.Omega) < 1 ? 0 : p.vhor / (p.Omega * radius_);
  p.vind = inducedVelocity(p);

  const double T = thrust(p);
  const double Q = torque(p);
  const double H = hforce_scale_ * hforce(p);

  double a0 = 0, a1s = 0, b1s = 0;
  if (hasFlapping()) {
    a0 = coning(p);
    a1s = longitudinalFlapping(p);
    b1s = lateralFlapping(p);
  }

  // The x-axis of BEM opposes the wind.
  const Eigen::Matrix3d rot =
      Eigen::AngleAxisd{std::atan2(p.vel[1], p.vel[0]), Eigen::Vector3d::UnitZ()}
          .toRotationMatrix();
  const int cw = p.spin();

  Wrench out;
  out.force = rot * Eigen::Vector3d{-(H + std::sin(a1s) * T),
                                    cw * std::sin(b1s) * T, -T * std::cos(a0)};
  out.torque = rot * Eigen::Vector3d{cw * k_ * b1s, k_ * a1s, -cw * Q};
  return out;
}

}  // namespace mybem
