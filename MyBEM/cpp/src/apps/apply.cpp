#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "mybem/csv.h"
#include "mybem/model.h"

using namespace mybem;
namespace fs = std::filesystem;

namespace {

/* Merged flight CSV, FLU. Column order per NeuroBEM/README.md. */
constexpr size_t kMergedCols = 29;
constexpr size_t kAngVel = 4;
constexpr size_t kVel = 14;
constexpr size_t kMot = 20;
constexpr size_t kDMot = 24;

int threadId() {
#ifdef _OPENMP
  return omp_get_thread_num();
#else
  return 0;
#endif
}

int threadCount() {
#ifdef _OPENMP
  return omp_get_max_threads();
#else
  return 1;
#endif
}

/* merged_<flight>_seg_X.csv -> <flight>_seg_X */
std::string segmentId(const fs::path& p) {
  const std::string s = p.stem().string();
  return s.rfind("merged_", 0) == 0 ? s.substr(7) : s;
}

bool applyOne(Model& model, const fs::path& in_path, const fs::path& out_path) {
  CsvReader in(in_path.string());
  if (in.cols() < kMergedCols) {
    printf("%s: expected at least %zu columns, found %zu\n",
           in_path.c_str(), kMergedCols, in.cols());
    return false;
  }
  CsvWriter out(out_path.string(), 7);

  State s;
  s.mot.resize(4);
  s.dmot.resize(4);
  double row[7];

  // Sequential on purpose: GSLHelper carries its bracket window across rows.
  for (size_t i = 0; i < in.rows(); ++i) {
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
  }
  printf("%s  %zu rows\n", out_path.filename().c_str(), in.rows());
  return true;
}

std::vector<fs::path> inputs(const fs::path& in) {
  if (!fs::is_directory(in)) return {in};
  std::vector<fs::path> found;
  for (const auto& e : fs::directory_iterator(in))
    if (e.path().extension() == ".csv" &&
        e.path().filename().string().rfind("merged_", 0) == 0)
      found.push_back(e.path());
  std::sort(found.begin(), found.end());
  return found;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 4) {
    printf("usage: mybem-apply MODEL.yaml INPUT PREDSDIR\n"
           "  INPUT     a merged_*_seg_*.csv or a directory of them\n"
           "  PREDSDIR  output root; files go to PREDSDIR/<model name>@<hash>/\n"
           "            existing outputs are skipped, so re-running resumes\n");
    return 1;
  }

  Model model = Model::load(argv[1]);
  const fs::path out_dir =
      fs::path(argv[3]) / (model.name() + "@" + model.hash());
  fs::create_directories(out_dir);
  model.save((out_dir / "params.yaml").string());

  std::vector<fs::path> todo;
  const std::vector<fs::path> all = inputs(argv[2]);
  for (const auto& p : all)
    if (!fs::exists(out_dir / (segmentId(p) + ".csv"))) todo.push_back(p);

#ifdef _OPENMP
  omp_set_max_active_levels(1);  // the rotor loop stays serial inside a file
#endif
  std::vector<Model> fleet;
  for (int i = 0; i < threadCount(); ++i) fleet.push_back(Model::load(argv[1]));

  printf("%zu/%zu segments | %d thread%s -> %s\n", todo.size(), all.size(),
         threadCount(), threadCount() == 1 ? "" : "s", out_dir.c_str());

  int failed = 0;
  const int n = static_cast<int>(todo.size());
#pragma omp parallel for schedule(dynamic) reduction(+ : failed)
  for (int i = 0; i < n; ++i)
    if (!applyOne(fleet[threadId()], todo[i],
                  out_dir / (segmentId(todo[i]) + ".csv")))
      failed++;

  if (failed) printf("%d segment%s failed\n", failed, failed == 1 ? "" : "s");
  return failed ? 1 : 0;
}
