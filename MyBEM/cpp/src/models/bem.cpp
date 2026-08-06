#include "mybem/models/bem.h"

#include <cstdio>
#include <cstdlib>

namespace mybem {
namespace {

double get(const Params& p, const std::string& key, double def) {
  auto it = p.find(key);
  return it == p.end() ? def : it->second;
}

std::string opt(const Options& o, const std::string& key,
                const std::string& def) {
  auto it = o.find(key);
  return it == o.end() ? def : it->second;
}

[[noreturn]] void fail(const std::string& msg) {
  printf("bem: %s\n", msg.c_str());
  exit(1);
}

}  // namespace

void BEMModel::load(const Params& p, const Options& o) {
  BEMParams& b = params_;

  b.cl = p.at("lift_coefficient");
  b.cd = p.at("drag_coefficient");
  b.theta0 = toRad(p.at("pitch"));
  b.theta1 = toRad(p.at("twist"));
  b.ci = p.at("chord_inner");
  b.co = p.at("chord_outer");
  b.cl_offset = p.at("lift_offset");
  b.rho = p.at("air_density");
  b.R = p.at("radius");
  b.b = p.at("num_blades");
  b.A = M_PI * b.R * b.R;

  k_ = p.at("hinge_spring_constant");
  hforce_scale_ = p.at("hforce_scale");
  radius_ = b.R;

  b.c = get(p, "chord_constant", 1.3e-2);
  b.cl1 = get(p, "stall_lift_slope", 8.529715);
  b.cl2 = get(p, "stall_lift_coefficient", 1.357441);
  b.Mstall = get(p, "stall_sigmoid_slope", 20);
  b.alpha0 = toRad(get(p, "stall_angle", 12));

  polar_ = opt(o, "polar", "sin_cos");
  chord_ = opt(o, "chord", "linear");

  if (polar_ == "sin_cos")
    polar_fn_ = &polarSinCos;
  else if (polar_ == "stall")
    polar_fn_ = &polarStall;
  else
    fail("unknown polar '" + polar_ + "' (sin_cos | stall)");

  if (chord_ == "linear")
    chord_fn_ = &chordLinear;
  else if (chord_ == "constant")
    chord_fn_ = &chordConstant;
  else
    fail("unknown chord '" + chord_ + "' (linear | constant)");

  solvers_.clear();
}

void BEMModel::reserve(size_t n_rotors) {
  if (n_rotors) _solver(n_rotors - 1);
}

GSLHelper& BEMModel::_solver(size_t index) {
  while (solvers_.size() <= index) {
    auto h = std::make_unique<GSLHelper>();
    static_cast<BEMParams&>(h->p) = params_;
    h->p.polar = polar_fn_;
    h->p.chord = chord_fn_;
    solvers_.push_back(std::move(h));
  }
  return *solvers_[index];
}

Params BEMModel::params() const {
  const BEMParams& b = params_;
  Params out{{"lift_coefficient", b.cl},
             {"drag_coefficient", b.cd},
             {"hinge_spring_constant", k_},
             {"lift_offset", b.cl_offset},
             {"hforce_scale", hforce_scale_},
             {"pitch", toDeg(b.theta0)},
             {"twist", toDeg(b.theta1)},
             {"chord_inner", b.ci},
             {"chord_outer", b.co},
             {"radius", b.R},
             {"num_blades", b.b},
             {"air_density", b.rho},
             {"chord_constant", b.c},
             {"stall_lift_slope", b.cl1},
             {"stall_lift_coefficient", b.cl2},
             {"stall_sigmoid_slope", b.Mstall},
             {"stall_angle", toDeg(b.alpha0)}};
  return out;
}

Options BEMModel::options() const {
  return {{"polar", polar_}, {"chord", chord_}};
}

/* Full set regardless of the active polar/chord; unread entries have no
 * effect. Defaults and bounds match the CMAES.cpp REGISTRY. */
std::vector<TunableParam> BEMModel::tunables() const {
  return {{"lift_coefficient", 15.24214, 0.5, 40},
          {"drag_coefficient", 13.54894, 0.5, 40},
          {"hinge_spring_constant", 5.89, 0.5, 30},
          {"lift_offset", 0, -0.2, 0.3},
          {"hforce_scale", 1, 0.1, 6},
          {"pitch", 21.77, 3, 40},
          {"twist", -11, -34, 6},
          {"chord_inner", 1.7e-2, 0.005, 0.04},
          {"chord_outer", 0.7e-2, 0.002, 0.03},
          {"radius", 0.064770, 0.04, 0.09},
          {"num_blades", 3, 2, 4},
          {"air_density", 1.204, 1.0, 1.4},
          {"chord_constant", 1.3e-2, 0.005, 0.04},
          {"stall_lift_slope", 8.529715, 0.5, 40},
          {"stall_lift_coefficient", 1.357441, 0.1, 20},
          {"stall_sigmoid_slope", 20, 1, 100},
          {"stall_angle", 12, 2, 40}};
}

void BEMModel::_setState(const PropState& p, double v1) {
  _solver(p.index)
      .setPropellerState(p.Omega, p.vtot, p.vel[2], p.alpha, p.mu, v1);
}

double BEMModel::inducedVelocity(const PropState& p) {
  _setState(p, 0);
  return _solver(p.index).solveInducedVelocity();
}

double BEMModel::thrust(const PropState& p) {
  _setState(p, p.vind);
  return _solver(p.index).integrateThrust();
}

double BEMModel::torque(const PropState& p) {
  _setState(p, p.vind);
  return _solver(p.index).integrateTorque();
}

double BEMModel::hforce(const PropState& p) {
  _setState(p, p.vind);
  return _solver(p.index).integrateHForce();
}

}  // namespace mybem
