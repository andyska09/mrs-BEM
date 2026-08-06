#include "mybem/models/simple.h"

namespace mybem {

void QuadraticModel::load(const Params& p, const Options&) {
  cl_ = p.at("lift_coefficient");
  cd_ = p.at("drag_coefficient");
}

Params QuadraticModel::params() const {
  return {{"lift_coefficient", cl_}, {"drag_coefficient", cd_}};
}

std::vector<TunableParam> QuadraticModel::tunables() const {
  return {{"lift_coefficient", 1.562522e-06, 1e-7, 1e-5},
          {"drag_coefficient", 1.908873e-08, 1e-9, 1e-7}};
}

}  // namespace mybem
