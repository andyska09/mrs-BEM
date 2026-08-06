#pragma once

#include "mybem/component.h"

namespace mybem {

/* An airframe plus an ordered list of additive components; the wrench is their
 * sum. Tunable keys are namespaced: "bem.lift_coefficient". */
class Model {
 public:
  static Model load(const std::string& path);
  void save(const std::string& path) const;

  Wrench evaluate(const State&);

  std::vector<TunableParam> tunables() const;
  Params values() const;
  void set(const Params& overrides);

  const std::string& name() const { return name_; }
  const std::string& drone() const { return drone_; }
  size_t size() const { return components_.size(); }

 private:
  std::string name_;
  std::string drone_;
  Airframe airframe_;
  std::vector<ComponentPtr> components_;
  std::vector<std::string> prefixes_;
};

}  // namespace mybem
