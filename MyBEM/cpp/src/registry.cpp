#include "mybem/models/bem.h"
#include "mybem/models/body_drag.h"
#include "mybem/models/motor_reaction.h"
#include "mybem/models/polyfit.h"
#include "mybem/models/simple.h"

namespace mybem {

ComponentPtr createComponent(const std::string& type) {
  if (type == "bem") return std::make_unique<BEMModel>();
  if (type == "quadratic") return std::make_unique<QuadraticModel>();
  if (type == "none") return std::make_unique<NoneModel>();
  if (type == "body_drag") return std::make_unique<BodyDrag>();
  if (type == "motor_reaction") return std::make_unique<MotorReaction>();
  if (type == "polyfit") return std::make_unique<PolyfitModel>();
  return nullptr;
}

std::vector<std::string> componentTypes() {
  return {"bem", "quadratic", "none", "body_drag", "motor_reaction", "polyfit"};
}

}  // namespace mybem
