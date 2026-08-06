#pragma once

#include <memory>

#include "mybem/types.h"

namespace mybem {

/* Additive force/torque contributor. Mirrors agi::ModelBase: every component
 * adds into the same wrench, the total is their sum. */
class Component {
 public:
  virtual ~Component() = default;

  virtual const char* type() const = 0;
  virtual void add(const State&, const Airframe&, Wrench&) = 0;
  virtual void load(const Params&) = 0;
  virtual Params params() const = 0;
  virtual std::vector<TunableParam> tunables() const = 0;

  virtual std::vector<std::string> diagnostics() const { return {}; }
  virtual void diagnose(const State&, const Airframe&, std::vector<double>&) {}
};

using ComponentPtr = std::unique_ptr<Component>;

ComponentPtr createComponent(const std::string& type);
std::vector<std::string> componentTypes();

}  // namespace mybem
