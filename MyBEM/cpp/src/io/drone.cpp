#include "mybem/drone.h"

#include <cstdio>
#include <cstdlib>
#include <sstream>

#include "mybem/yaml.h"

namespace mybem {
namespace {

[[noreturn]] void fail(const std::string& path, const std::string& msg) {
  printf("%s: %s\n", path.c_str(), msg.c_str());
  exit(1);
}

/* "[a, b, c]" -> three doubles. */
Eigen::Vector3d vec3(const std::string& path, const std::string& s) {
  std::string body = s;
  if (body.size() >= 2 && body.front() == '[' && body.back() == ']')
    body = body.substr(1, body.size() - 2);
  std::stringstream ss(body);
  std::string cell;
  std::vector<double> v;
  while (std::getline(ss, cell, ',')) v.push_back(std::stod(cell));
  if (v.size() != 3) fail(path, "inertia: expected 3 numbers, got " + s);
  return {v[0], v[1], v[2]};
}

}  // namespace

Drone Drone::load(const std::string& path) {
  const YamlDoc doc = readYaml(path);
  Drone d;
  d.name = doc.get("name");
  const std::string mass = doc.get("mass");
  if (mass.empty()) fail(path, "no 'mass'");
  d.mass = std::stod(mass);
  const std::string inertia = doc.get("inertia");
  if (inertia.empty()) fail(path, "no 'inertia'");
  d.inertia = vec3(path, inertia);
  return d;
}

}  // namespace mybem
