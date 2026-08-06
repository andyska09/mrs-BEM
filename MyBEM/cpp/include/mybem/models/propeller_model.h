#pragma once

#include "mybem/component.h"
#include "mybem/prop_state.h"

namespace mybem {

/* Base for every per-propeller aerodynamic model. Owns the kinematics, the
 * force/torque assembly and the summation over rotors; subclasses supply only
 * thrust, drag torque, h-force and induced velocity. */
class PropellerModel : public Component {
 public:
  void add(const State&, const Airframe&, Wrench&) override;

 protected:
  virtual double thrust(const PropState&) = 0;
  virtual double torque(const PropState&) = 0;
  virtual double hforce(const PropState&) = 0;
  virtual double inducedVelocity(const PropState&) = 0;
  virtual bool hasFlapping() const = 0;

  Wrench evaluate(PropState&);

  double radius_ = 5.1 * 2.54 / 2 * 1e-2;
  double k_ = 5.89;
  double hforce_scale_ = 1.0;
  bool distortion_ = false;
};

}  // namespace mybem
