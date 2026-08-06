#pragma once

#include <string>

#include "mybem/bem/gsl_helper.h"
#include "mybem/models/propeller_model.h"

namespace mybem {

/* Non-linear blade element momentum model. POLAR / CHORD / DIST are runtime
 * options here, not #defines. */
class BEMModel : public PropellerModel {
 public:
  const char* type() const override { return "bem"; }
  void load(const Params&, const Options&) override;
  Params params() const override;
  Options options() const override;
  std::vector<TunableParam> tunables() const override;

 protected:
  double thrust(const PropState&) override;
  double torque(const PropState&) override;
  double hforce(const PropState&) override;
  double inducedVelocity(const PropState&) override;
  bool hasFlapping() const override { return true; }

 private:
  /* One solver per rotor: GSLHelper permanently widens its bracket window on a
   * failed bracketing, so a shared one would couple the rotors. */
  GSLHelper& _solver(size_t index);
  void _setState(const PropState&, double v1);

  BEMParams params_;
  PolarFn polar_fn_ = &polarSinCos;
  ChordFn chord_fn_ = &chordLinear;
  std::vector<std::unique_ptr<GSLHelper>> solvers_;
  std::string polar_ = "sin_cos";
  std::string chord_ = "linear";
  std::string distortion_name_ = "off";
};

}  // namespace mybem
