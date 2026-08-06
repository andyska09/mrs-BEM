#include "mybem/models/motor_reaction.h"

namespace mybem {

void MotorReaction::add(const State& s, const Airframe&, Wrench& w) {
  const std::vector<bool>& cw = Airframe::spinCW();
  for (size_t i = 0; i < s.dmot.size() && i < cw.size(); ++i)
    w.torque[2] -= (cw[i] ? 1.0 : -1.0) * s.dmot[i] * inertia_;
}

void MotorReaction::load(const Params& p, const Options&) {
  inertia_ = p.at("motor_inertia");
}

Params MotorReaction::params() const { return {{"motor_inertia", inertia_}}; }

std::vector<TunableParam> MotorReaction::tunables() const {
  return {{"motor_inertia", 9.3575e-06, 1e-6, 1e-4}};
}

}  // namespace mybem
