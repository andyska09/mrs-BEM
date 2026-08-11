#pragma once

#include "mybem/component.h"

namespace mybem {

/* An airframe plus an ordered list of additive components; the wrench is their
 * sum. Tunable keys are namespaced: "bem.lift_coefficient". */
class Model {
 public:
  static Model load(const std::string& path);
  void save(const std::string& path) const;

  /* The resolved config, and the 6-hex id of it that names a preds folder. */
  std::string text() const;
  std::string hash() const;

  Wrench evaluate(const State&);

  std::vector<TunableParam> tunables() const;
  Params values() const;
  void set(const Params& overrides);

  const std::string& name() const { return name_; }
  void setName(const std::string& name) { name_ = name; }
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
