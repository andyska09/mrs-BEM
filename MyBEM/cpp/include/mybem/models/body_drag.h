#pragma once

#include "mybem/component.h"

namespace mybem {

/* Airframe drag, uniform coefficient in the xy-plane with per-axis frontal
 * area. Force only, no torque. */
class BodyDrag : public Component {
 public:
  const char* type() const override { return "body_drag"; }
  void add(const State&, const Airframe&, Wrench&) override;
  void load(const Params&, const Options&) override;
  Params params() const override;
  std::vector<TunableParam> tunables() const override;

 private:
  double rho_ = 1.204;
  double cxy_ = 1.0;
  double cz_ = 1.0;
  double ax_ = 0.06 * 0.09;
  double ay_ = 0.10 * 0.09;
  double az_ = 0.10 * 0.06;
};

}  // namespace mybem
