// CMA-ES aero fit. Optimizes (cl,cd) on a force-only normalized MSE; k fixed.
//   ./cmaes subset.csv           report at default params
//   ./cmaes subset.csv cl cd k   report at given params
//   ./cmaes subset.csv --cma     fit (cl,cd); writes convergence.csv
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

static constexpr double MASS = 0.772;
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

static void mse(Quadcopter& quad, const std::vector<Sample>& data,
                Array3d& fmse, Array3d& tmse) {
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
  fmse = fse / data.size();
  tmse = tse / data.size();
}

// force-only loss; torque is the NN's job and is excluded from the fit
static double loss(Quadcopter& quad, const std::vector<Sample>& data,
                   const Vector3d& p, double sf) {
  quad.setAero(p[0], p[1], p[2]);
  Array3d fm, tm;
  mse(quad, data, fm, tm);
  return fm.sum() / (sf * sf);
}

static void report(Quadcopter& quad, const std::vector<Sample>& data,
                   const Vector3d& p) {
  quad.setAero(p[0], p[1], p[2]);
  Array3d fm, tm;
  mse(quad, data, fm, tm);
  Array3d fr = fm.sqrt(), tr = tm.sqrt();
  printf("cl=%.4f cd=%.4f k=%.4f\n", p[0], p[1], p[2]);
  printf("force  RMSE: %.4f %.4f %.4f\n", fr[0], fr[1], fr[2]);
  printf("torque RMSE: %.5f %.5f %.5f\n", tr[0], tr[1], tr[2]);
}

static Vector3d cma(Quadcopter& quad, const std::vector<Sample>& data, double sf) {
  using Vec = Eigen::Vector2d;
  using Mat = Eigen::Matrix2d;
  const Vec DEF2{DEFAULT[0], DEFAULT[1]};
  const double KFIX = DEFAULT[2];

  const int N = 2, lambda = 8, mu = 4, maxgen = 200;
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

  Vec mean = Vec::Ones();  // normalized coords: param = mean .* DEF2
  double sigma = 0.3;
  Mat C = Mat::Identity();
  Vec ps = Vec::Zero(), pc = Vec::Zero();
  std::mt19937 rng(0);
  std::normal_distribution<double> randn(0.0, 1.0);

  Vec best_x = mean;
  double best_f = 1e18;
  std::ofstream log("convergence.csv");
  log << "gen,loss,cl,cd,k\n";

  for (int gen = 0; gen < maxgen; ++gen) {
    Eigen::SelfAdjointEigenSolver<Mat> es(C);
    Mat B = es.eigenvectors();
    Vec D = es.eigenvalues().cwiseMax(1e-12).cwiseSqrt();

    std::vector<Vec> zs(lambda), ys(lambda), xs(lambda);
    std::vector<std::pair<double, int>> fit(lambda);
    for (int k = 0; k < lambda; ++k) {
      Vec z{randn(rng), randn(rng)};
      Vec y = B * (D.asDiagonal() * z);
      Vec x = mean + sigma * y;
      Vec xc = x.cwiseMax(0.05).cwiseMin(5.0);   // box constraint
      double pen = (x - xc).squaredNorm();
      Vector3d p{xc[0] * DEF2[0], xc[1] * DEF2[1], KFIX};
      double f = loss(quad, data, p, sf) + 1e3 * pen;
      zs[k] = z; ys[k] = y; xs[k] = x;
      fit[k] = {f, k};
    }
    std::sort(fit.begin(), fit.end());

    Vec zmean = Vec::Zero(), ymean = Vec::Zero();
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

    Mat rank_mu = Mat::Zero();
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
    double bcl = best_x[0] * DEF2[0], bcd = best_x[1] * DEF2[1];
    log << gen << ',' << best_f << ',' << bcl << ',' << bcd << ',' << KFIX << '\n';
    if (gen % 10 == 0)
      printf("gen %3d  loss %.5f  cl=%.3f cd=%.3f k=%.3f(fixed)  sigma=%.4f\n", gen,
             best_f, bcl, bcd, KFIX, sigma);
    if (sigma < 1e-4) break;
  }
  log.close();
  return Vector3d{best_x[0] * DEF2[0], best_x[1] * DEF2[1], KFIX};
}

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "usage: %s subset.csv [cl cd k | --cma]\n", argv[0]);
    return 1;
  }
  std::vector<Sample> data = load(argv[1]);
  Quadcopter quad;

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
    Vector3d best = cma(quad, data, sf);
    printf("--- best ---\n");
    report(quad, data, best);
  } else if (argc >= 5) {
    report(quad, data, {std::stod(argv[2]), std::stod(argv[3]), std::stod(argv[4])});
  } else {
    report(quad, data, DEFAULT);
  }
  return 0;
}
