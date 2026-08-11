#pragma once

#include <array>
#include <string>
#include <utility>
#include <vector>

#include "mybem/component.h"

namespace mybem {

/* Vehicle-level polynomial gray-box model (Sun, de Visser & Chu, JoA 56(2) 2019).
 * Terms and coefficients are identified by `python -m mybem.polyfit` and read
 * from the file named by the `coeffs` option; only the four scalars are tuned. */
class PolyfitModel : public Component {
 public:
  const char* type() const override { return "polyfit"; }
  void add(const State&, const Airframe&, Wrench&) override;
  void load(const Params&, const Options&) override;
  Params params() const override;
  Options options() const override;
  std::vector<TunableParam> tunables() const override;
  std::vector<std::string> pathOptions() const override { return {"coeffs"}; }

 private:
  struct Term {
    double coeff;
    std::vector<std::pair<int, int>> powers;
  };

  double radius_ = 0.06477;
  double rho_ = 1.204;
  double ct_hover_ = 0.02187;
  double ref_length_ = 0.12682;

  std::string coeffs_;
  bool loaded_ = false;
  std::vector<double> edges_{0.0, 30.0, 60.0, 90.0};
  /* [axis][bin]; axis order is Cx, Cy, Cz, Cl, Cm, Cn. */
  std::array<std::vector<std::vector<Term>>, 6> axes_;
};

}  // namespace mybem
