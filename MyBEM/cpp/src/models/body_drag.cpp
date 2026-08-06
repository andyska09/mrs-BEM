#include "mybem/models/body_drag.h"

namespace mybem {

void BodyDrag::add(const State& s, const Airframe&, Wrench& w) {
  Eigen::Vector3d drag = -0.5 * rho_ * s.vel * s.vel.norm();
  drag[0] *= cxy_ * ax_;
  drag[1] *= cxy_ * ay_;
  drag[2] *= cz_ * az_;
  w.force += drag;
}

void BodyDrag::load(const Params& p, const Options&) {
  rho_ = p.at("air_density");
  cxy_ = p.at("horizontal_drag_coefficient");
  cz_ = p.at("vertical_drag_coefficient");
  ax_ = p.at("frontarea_x");
  ay_ = p.at("frontarea_y");
  az_ = p.at("frontarea_z");
}

Params BodyDrag::params() const {
  return {{"air_density", rho_},
          {"horizontal_drag_coefficient", cxy_},
          {"vertical_drag_coefficient", cz_},
          {"frontarea_x", ax_},
          {"frontarea_y", ay_},
          {"frontarea_z", az_}};
}

std::vector<TunableParam> BodyDrag::tunables() const {
  return {{"horizontal_drag_coefficient", 1.0, 0.1, 5.0},
          {"vertical_drag_coefficient", 1.0, 0.1, 5.0},
          {"frontarea_x", 0.06 * 0.09, 0.0, 0.03},
          {"frontarea_y", 0.10 * 0.09, 0.0, 0.03},
          {"frontarea_z", 0.10 * 0.06, 0.0, 0.03},
          {"air_density", 1.204, 1.0, 1.4}};
}

}  // namespace mybem
