#pragma once

#include "mybem/models/propeller_model.h"

namespace mybem {

/* No propeller model: zero force and zero torque. NOTE: MODEL -1 still ran the
 * flapping angles, so it emitted a hinge-spring torque with zero thrust. */
class NoneModel : public PropellerModel {
 public:
  const char* type() const override { return "none"; }
  void load(const Params&, const Options&) override {}
  Params params() const override { return {}; }
  std::vector<TunableParam> tunables() const override { return {}; }

 protected:
  double thrust(const PropState&) override { return 0; }
  double torque(const PropState&) override { return 0; }
  double hforce(const PropState&) override { return 0; }
  double inducedVelocity(const PropState&) override { return 0; }
  bool hasFlapping() const override { return false; }
};

/* Quadratic fit: T = cl w^2, Q = cd w^2. No h-force, no vind, no flapping. */
class QuadraticModel : public PropellerModel {
 public:
  const char* type() const override { return "quadratic"; }
  void load(const Params&, const Options&) override;
  Params params() const override;
  std::vector<TunableParam> tunables() const override;

 protected:
  double thrust(const PropState& p) override { return cl_ * p.Omega * p.Omega; }
  double torque(const PropState& p) override { return cd_ * p.Omega * p.Omega; }
  double hforce(const PropState&) override { return 0; }
  double inducedVelocity(const PropState&) override { return 0; }
  bool hasFlapping() const override { return false; }

 private:
  double cl_ = 1.562522e-06;
  double cd_ = 1.908873e-08;
};

}  // namespace mybem
