#include "mybem/models/polyfit.h"

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

namespace mybem {
namespace {

constexpr int NBASE = 12;
constexpr int NVARS = 2 * NBASE;

const char* const VARS[NBASE] = {"mu_x", "mu_y", "mu_z",  "p_bar",
                                 "q_bar", "r_bar", "u_p",  "u_q",
                                 "u_r",  "nu_in", "mu_h2", "lam2"};
const char* const AXES[6] = {"Cx", "Cy", "Cz", "Cl", "Cm", "Cn"};

[[noreturn]] void fail(const std::string& file, int line, const std::string& m) {
  std::cerr << file << ":" << line << ": " << m << "\n";
  std::exit(1);
}

int varIndex(const std::string& n) {
  const bool abs = n.rfind("abs_", 0) == 0;
  const std::string base = abs ? n.substr(4) : n;
  for (int i = 0; i < NBASE; ++i)
    if (base == VARS[i]) return abs ? i + NBASE : i;
  return -1;
}

int axisIndex(const std::string& n) {
  for (int i = 0; i < 6; ++i)
    if (n == AXES[i]) return i;
  return -1;
}

double ipow(double x, int k) {
  double r = 1.0;
  while (k-- > 0) r *= x;
  return r;
}

}  // namespace

void PolyfitModel::load(const Params& p, const Options& o) {
  radius_ = p.at("radius");
  rho_ = p.at("air_density");
  ct_hover_ = p.at("ct_hover");
  ref_length_ = p.at("ref_length");
  if (loaded_ && o.at("coeffs") == coeffs_) return;
  coeffs_ = o.at("coeffs");

  std::ifstream in(coeffs_);
  if (!in) fail(coeffs_, 0, "cannot open polyfit coefficients");

  std::string line;
  int no = 0;
  bool got_edges = false;
  for (auto& a : axes_) a.clear();

  while (std::getline(in, line)) {
    ++no;
    const size_t hash = line.find('#');
    if (hash != std::string::npos) line.resize(hash);
    std::istringstream ss(line);
    std::string head;
    if (!(ss >> head)) continue;

    if (head == "beta_edges") {
      edges_.clear();
      for (double v; ss >> v;) edges_.push_back(v);
      if (edges_.size() < 2) fail(coeffs_, no, "beta_edges needs >= 2 values");
      for (auto& a : axes_) a.resize(edges_.size() - 1);
      got_edges = true;
      continue;
    }

    const int axis = axisIndex(head);
    if (axis < 0) fail(coeffs_, no, "unknown axis '" + head + "'");
    if (!got_edges) fail(coeffs_, no, "beta_edges must come first");

    int bin;
    std::string name;
    double coeff;
    if (!(ss >> bin >> name >> coeff))
      fail(coeffs_, no, "expected: <axis> <bin> <term> <coefficient>");
    if (bin < 0 || bin + 1 >= static_cast<int>(edges_.size()))
      fail(coeffs_, no, "bin out of range");

    Term t{coeff, {}};
    if (name != "1") {
      for (size_t i = 0; i <= name.size();) {
        const size_t star = name.find('*', i);
        const std::string part = name.substr(
            i, star == std::string::npos ? std::string::npos : star - i);
        const size_t caret = part.find('^');
        const int v = varIndex(caret == std::string::npos ? part
                                                          : part.substr(0, caret));
        if (v < 0) fail(coeffs_, no, "unknown variable in '" + name + "'");
        t.powers.emplace_back(
            v, caret == std::string::npos ? 1 : std::stoi(part.substr(caret + 1)));
        if (star == std::string::npos) break;
        i = star + 1;
      }
    }
    axes_[axis][bin].push_back(std::move(t));
  }

  for (auto& a : axes_) a.resize(edges_.size() - 1);
  loaded_ = true;
}

void PolyfitModel::add(const State& s, const Airframe& af, Wrench& w) {
  const size_t n = s.mot.size();
  double sum2 = 0.0;
  for (double m : s.mot) sum2 += m * m;
  const double ob = std::sqrt(sum2 / static_cast<double>(n));
  if (!(ob > 0.0)) return;
  const double tip = ob * radius_;

  double v[NVARS];
  for (int i = 0; i < 3; ++i) {
    v[i] = s.vel[i] / tip;
    v[3 + i] = s.omega[i] * ref_length_ / tip;
  }

  const std::vector<Eigen::Vector3d> off = af.offsets();
  const std::vector<bool>& cw = Airframe::spinCW();
  v[6] = v[7] = v[8] = 0.0;
  for (size_t i = 0; i < n; ++i) {
    const double w2 = s.mot[i] * s.mot[i] / (ob * ob);
    v[6] -= std::copysign(1.0, off[i][1]) * w2;
    v[7] += std::copysign(1.0, off[i][0]) * w2;
    v[8] += (cw[i] ? 1.0 : -1.0) * w2;
  }

  const double h = v[0] * v[0] + v[1] * v[1];
  double nu = std::sqrt(ct_hover_ / 2.0);
  for (int i = 0; i < 64; ++i)
    nu = ct_hover_ / (2.0 * std::sqrt(h + (nu - v[2]) * (nu - v[2])));
  v[9] = nu;
  v[10] = h;
  v[11] = (nu - v[2]) * (nu - v[2]);
  for (int i = 0; i < NBASE; ++i) v[NBASE + i] = std::fabs(v[i]);

  const double mh = std::sqrt(h);
  const double beta =
      mh > 1e-9 ? toDeg(std::asin(std::min(1.0, v[NBASE + 1] / mh))) : 0.0;
  size_t bin = 0;
  while (bin + 2 < edges_.size() && beta >= edges_[bin + 1]) ++bin;

  double c[6] = {0, 0, 0, 0, 0, 0};
  for (int a = 0; a < 6; ++a)
    for (const Term& t : axes_[a][bin]) {
      double x = t.coeff;
      for (const auto& pk : t.powers) x *= ipow(v[pk.first], pk.second);
      c[a] += x;
    }

  const double q = rho_ * static_cast<double>(n) * M_PI * radius_ * radius_ * tip * tip;
  w.force += Eigen::Vector3d(c[0], c[1], c[2]) * q;
  w.torque += Eigen::Vector3d(c[3], c[4], c[5]) * (q * ref_length_);
}

Params PolyfitModel::params() const {
  return {{"radius", radius_},
          {"air_density", rho_},
          {"ct_hover", ct_hover_},
          {"ref_length", ref_length_}};
}

Options PolyfitModel::options() const { return {{"coeffs", coeffs_}}; }

std::vector<TunableParam> PolyfitModel::tunables() const {
  return {{"radius", 0.06477, 0.05, 0.08},
          {"air_density", 1.204, 1.0, 1.4},
          {"ct_hover", 0.02187, 0.01, 0.04},
          {"ref_length", 0.12682, 0.05, 0.25}};
}

}  // namespace mybem
