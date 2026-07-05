// Parameter identification tool.
//   ./identify subset.csv              -> report RMSE at default cl,cd,k
//   ./identify subset.csv cl cd k      -> report RMSE at given cl,cd,k
//   ./identify subset.csv --cma        -> CMA-ES fit of (cl,cd,k); writes convergence.csv
//
// Loads the fixed subset once, precomputes measured force/torque, and optimizes
// a normalized force+torque RMSE loss. CMA-ES works in normalized coordinates
// (param / default) so the differing scales of cl, cd, k are handled cleanly.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include <eigen3/Eigen/Dense>

#include "params.h"
#include "quadcopter.h"

using Eigen::Array3d;
using Eigen::Matrix3d;
using Eigen::Vector3d;

static constexpr double MASS = 0.752;
static const Vector3d INERTIA{0.00254, 0.00214, 0.00436};
static const Vector3d DEFAULT{15.24214, 13.54894, 5.89};  // cl, cd, k (params.h)

struct Sample {
  Vector3d ang_acc, ang_vel, acc, vel, pos, fmeas, tmeas;
  Eigen::Quaterniond quat;
  std::vector<double> omega;
};

static std::vector<Sample> load(const std::string& path) {
  std::vector<Sample> out;
  std::ifstream file(path);
  if (!file) {
    fprintf(stderr, "cannot open %s\n", path.c_str());
    exit(1);
  }
  std::string line, cell;
  while (std::getline(file, line)) {
    std::stringstream ss(line);
    std::vector<double> v;
    while (std::getline(ss, cell, ',')) v.push_back(std::stod(cell));
    if (v.size() < 28) continue;
    Sample s;
    s.ang_acc = {v[1], v[2], v[3]};
    s.ang_vel = {v[4], v[5], v[6]};
    s.quat = Eigen::Quaterniond(v[10], v[7], v[8], v[9]);  // w, x, y, z
    s.acc = {v[11], v[12], v[13]};
    s.vel = {v[14], v[15], v[16]};
    s.pos = {v[17], v[18], v[19]};
    s.omega = {v[20], v[21], v[22], v[23]};
    s.fmeas = MASS * s.acc;
    s.tmeas = INERTIA.cwiseProduct(s.ang_acc) +
              s.ang_vel.cross(INERTIA.cwiseProduct(s.ang_vel));
    out.push_back(s);
  }
  return out;
}

// per-axis force and torque RMSE at the aero params currently set on quad
static void rmse(Quadcopter& quad, const std::vector<Sample>& data,
                 Array3d& frmse, Array3d& trmse) {
  Array3d fse = Array3d::Zero(), tse = Array3d::Zero();
  for (const Sample& s : data) {
    quad.setAcceleration(flu2frd(s.acc));
    quad.setVelocity(flu2frd(s.vel));
    quad.setPosition(flu2frd(s.pos));
    quad.setAngularAcceleration(flu2frd(s.ang_acc));
    quad.setAngularVelocity(flu2frd(s.ang_vel));
    quad.setQuaternionAttitude(flu2frd(s.quat));
    quad.setMotorOmega(s.omega);
    fse += (frd2flu(quad.getForce()) - s.fmeas).array().square();
    tse += (frd2flu(quad.getTorque()) - s.tmeas).array().square();
  }
  frmse = (fse / data.size()).sqrt();
  trmse = (tse / data.size()).sqrt();
}

// scalar loss: relative combined force RMSE + relative combined torque RMSE
static double loss(Quadcopter& quad, const std::vector<Sample>& data,
                   const Vector3d& p, double sf, double st) {
  quad.setAero(p[0], p[1], p[2]);
  Array3d fr, tr;
  rmse(quad, data, fr, tr);
  return std::sqrt(fr.square().mean()) / sf + std::sqrt(tr.square().mean()) / st;
}

static void report(Quadcopter& quad, const std::vector<Sample>& data,
                   const Vector3d& p) {
  quad.setAero(p[0], p[1], p[2]);
  Array3d fr, tr;
  rmse(quad, data, fr, tr);
  printf("cl=%.4f cd=%.4f k=%.4f\n", p[0], p[1], p[2]);
  printf("force  RMSE: %.4f %.4f %.4f\n", fr[0], fr[1], fr[2]);
  printf("torque RMSE: %.5f %.5f %.5f\n", tr[0], tr[1], tr[2]);
}

static Vector3d cma(Quadcopter& quad, const std::vector<Sample>& data, double sf,
                    double st) {
  const int N = 3, lambda = 10, mu = 5, maxgen = 200;
  Eigen::VectorXd w(mu);
  for (int i = 0; i < mu; ++i) w[i] = std::log(mu + 0.5) - std::log(i + 1.0);
  w /= w.sum();
  const double mueff = 1.0 / w.squaredNorm();
  const double cc = (4 + mueff / N) / (N + 4 + 2 * mueff / N);
  const double cs = (mueff + 2) / (N + mueff + 5);
  const double c1 = 2.0 / ((N + 1.3) * (N + 1.3) + mueff);
  const double cmu =
    std::min(1 - c1, 2 * (mueff - 2 + 1 / mueff) / ((N + 2) * (N + 2) + mueff));
  const double ds =
    1 + 2 * std::max(0.0, std::sqrt((mueff - 1.0) / (N + 1)) - 1) + cs;
  const double chiN = std::sqrt((double)N) * (1 - 1.0 / (4 * N) + 1.0 / (21.0 * N * N));

  Vector3d mean = Vector3d::Ones();  // normalized: param = mean .* DEFAULT
  double sigma = 0.3;
  Matrix3d C = Matrix3d::Identity();
  Vector3d ps = Vector3d::Zero(), pc = Vector3d::Zero();
  std::mt19937 rng(0);
  std::normal_distribution<double> randn(0.0, 1.0);

  Vector3d best_x = mean;
  double best_f = 1e18;
  std::ofstream log("convergence.csv");
  log << "gen,loss,cl,cd,k\n";

  for (int gen = 0; gen < maxgen; ++gen) {
    Eigen::SelfAdjointEigenSolver<Matrix3d> es(C);
    Matrix3d B = es.eigenvectors();
    Vector3d D = es.eigenvalues().cwiseMax(1e-12).cwiseSqrt();

    std::vector<Vector3d> zs(lambda), ys(lambda), xs(lambda);
    std::vector<std::pair<double, int>> fit(lambda);
    for (int k = 0; k < lambda; ++k) {
      Vector3d z{randn(rng), randn(rng), randn(rng)};
      Vector3d y = B * (D.asDiagonal() * z);
      Vector3d x = mean + sigma * y;
      Vector3d xc = x.cwiseMax(0.05).cwiseMin(5.0);   // plausible box, normalized
      double pen = (x - xc).squaredNorm();
      double f = loss(quad, data, xc.cwiseProduct(DEFAULT), sf, st) + 1e3 * pen;
      zs[k] = z; ys[k] = y; xs[k] = x;
      fit[k] = {f, k};
    }
    std::sort(fit.begin(), fit.end());

    Vector3d meanold = mean, zmean = Vector3d::Zero(), ymean = Vector3d::Zero();
    mean.setZero();
    for (int i = 0; i < mu; ++i) {
      int k = fit[i].second;
      zmean += w[i] * zs[k];
      ymean += w[i] * ys[k];
      mean += w[i] * xs[k];
    }

    ps = (1 - cs) * ps + std::sqrt(cs * (2 - cs) * mueff) * (B * zmean);
    double hsig = ps.norm() / std::sqrt(1 - std::pow(1 - cs, 2.0 * (gen + 1))) / chiN
                  < 1.4 + 2.0 / (N + 1) ? 1.0 : 0.0;
    pc = (1 - cc) * pc + hsig * std::sqrt(cc * (2 - cc) * mueff) * ymean;

    Matrix3d rank_mu = Matrix3d::Zero();
    for (int i = 0; i < mu; ++i)
      rank_mu += w[i] * ys[fit[i].second] * ys[fit[i].second].transpose();
    C = (1 - c1 - cmu) * C + c1 * (pc * pc.transpose() + (1 - hsig) * cc * (2 - cc) * C)
        + cmu * rank_mu;
    C = (C + C.transpose()) / 2;
    sigma *= std::exp((cs / ds) * (ps.norm() / chiN - 1));

    if (fit[0].first < best_f) {
      best_f = fit[0].first;
      best_x = xs[fit[0].second].cwiseMax(0.05).cwiseMin(5.0);
    }
    Vector3d bp = best_x.cwiseProduct(DEFAULT);
    log << gen << ',' << best_f << ',' << bp[0] << ',' << bp[1] << ',' << bp[2] << '\n';
    if (gen % 10 == 0)
      printf("gen %3d  loss %.5f  cl=%.3f cd=%.3f k=%.3f  sigma=%.4f\n", gen,
             best_f, bp[0], bp[1], bp[2], sigma);
    if (sigma < 1e-4) break;
  }
  log.close();
  return best_x.cwiseProduct(DEFAULT);
}

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "usage: %s subset.csv [cl cd k | --cma]\n", argv[0]);
    return 1;
  }
  std::vector<Sample> data = load(argv[1]);
  Quadcopter quad;

  // reference scales for the normalized loss (measured RMS magnitudes)
  double sf2 = 0, st2 = 0;
  for (const Sample& s : data) {
    sf2 += s.fmeas.squaredNorm();
    st2 += s.tmeas.squaredNorm();
  }
  double sf = std::sqrt(sf2 / data.size()), st = std::sqrt(st2 / data.size());

  bool do_cma = argc >= 3 && std::string(argv[2]) == "--cma";
  if (do_cma) {
    printf("loaded %zu rows | sf=%.3f st=%.4f\n", data.size(), sf, st);
    printf("--- baseline (default params) ---\n");
    report(quad, data, DEFAULT);
    printf("--- CMA-ES ---\n");
    Vector3d best = cma(quad, data, sf, st);
    printf("--- best ---\n");
    report(quad, data, best);
  } else if (argc >= 5) {
    report(quad, data, {std::stod(argv[2]), std::stod(argv[3]), std::stod(argv[4])});
  } else {
    report(quad, data, DEFAULT);
  }
  return 0;
}
