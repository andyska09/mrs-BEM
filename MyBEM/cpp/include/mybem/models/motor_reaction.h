#pragma once

#include "mybem/component.h"

namespace mybem {

/* Body-z reaction torque from propeller angular acceleration, driven by the
 * dmot columns. Never active in the original pipeline: simulator.cpp read
 * dmot but never called setMotorAcceleration, so dOmega was always 0. */
class MotorReaction : public Component {
 public:
  const char* type() const override { return "motor_reaction"; }
  void add(const State&, const Airframe&, Wrench&) override;
  void load(const Params&) override;
  Params params() const override;
  std::vector<TunableParam> tunables() const override;

 private:
  double inertia_ = 9.3575e-06;
};

}  // namespace mybem
