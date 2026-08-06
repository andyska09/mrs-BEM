#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

#include "mybem/csv.h"
#include "mybem/model.h"

using namespace mybem;

namespace {

/* Merged flight CSV, FLU. Column order per NeuroBEM/README.md. */
constexpr size_t kMergedCols = 29;
constexpr size_t kAngVel = 4;
constexpr size_t kVel = 14;
constexpr size_t kMot = 20;
constexpr size_t kDMot = 24;

}  // namespace

int main(int argc, char** argv) {
  if (argc != 4) {
    printf("usage: mybem-apply MODEL.yaml INPUT.csv OUTPUT.csv\n");
    return 1;
  }

  Model model = Model::load(argv[1]);

  // Record the effective config next to the output, as the original did.
  const std::string out_path(argv[3]);
  const size_t slash = out_path.find_last_of('/');
  model.save((slash == std::string::npos ? "" : out_path.substr(0, slash + 1)) +
             "params.yaml");

  CsvReader in(argv[2]);
  if (in.cols() < kMergedCols) {
    printf("%s: expected at least %zu columns, found %zu\n", argv[2],
           kMergedCols, in.cols());
    return 1;
  }

  CsvWriter out(argv[3], 7);

  State s;
  s.mot.resize(4);
  s.dmot.resize(4);
  double row[7];

  double sum = 0, sum_sq = 0;

  // Sequential on purpose: GSLHelper carries its bracket window across rows.
  for (size_t i = 0; i < in.rows(); ++i) {
    const auto t0 = std::chrono::steady_clock::now();
    const double* d = in.row(i);
    s.vel = flu2frd(Eigen::Vector3d{d[kVel], d[kVel + 1], d[kVel + 2]});
    s.omega =
        flu2frd(Eigen::Vector3d{d[kAngVel], d[kAngVel + 1], d[kAngVel + 2]});
    memcpy(s.mot.data(), d + kMot, 4 * sizeof(double));
    memcpy(s.dmot.data(), d + kDMot, 4 * sizeof(double));

    const Wrench w = model.evaluate(s);
    const Eigen::Vector3d f = frd2flu(w.force);
    const Eigen::Vector3d t = frd2flu(w.torque);

    row[0] = d[0];
    memcpy(row + 1, f.data(), 3 * sizeof(double));
    memcpy(row + 4, t.data(), 3 * sizeof(double));
    out.add(row);

    const double dt = std::chrono::duration<double, std::micro>(
                          std::chrono::steady_clock::now() - t0)
                          .count();
    sum += dt;
    sum_sq += dt * dt;
  }

  const double n = static_cast<double>(in.rows());
  const double mean = sum / n;
  printf("%s: %zu rows, %.1f +- %.2f us/row\n", argv[3], in.rows(), mean,
         std::sqrt(sum_sq / n - mean * mean));
  return 0;
}
