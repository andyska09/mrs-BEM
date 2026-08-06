#pragma once

#include <string>

#include "mybem/bem/gsl_helper.h"
#include "mybem/models/propeller_model.h"

namespace mybem {

/* Non-linear blade element momentum model. The former POLAR / CHORD / DIST
 * preprocessor switches are runtime options; every parameter field always
 * exists regardless of which are selected. */
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
  void _setState(const PropState&, double v1);

  GSLHelper solver_;
  std::string polar_ = "sin_cos";
  std::string chord_ = "linear";
  std::string distortion_name_ = "off";
};

}  // namespace mybem
