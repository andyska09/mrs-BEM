#pragma once

#include <Eigen/Dense>
#include <random>
#include <vector>

namespace mybem {

struct Offspring {
  Eigen::VectorXd normal;  // z ~ N(0, I)
  Eigen::VectorXd step;    // correlated step B*D*z in the covariance geometry
  Eigen::VectorXd point;   // candidate x = mean + sigma*step
};

/* (mu/mu_w, lambda)-CMA-ES over an unbounded x-space. Knows nothing about the
 * objective: bounds and scaling belong to the caller. */
class Cma {
 public:
  Cma(int dim, unsigned seed);

  std::vector<Offspring> sample();
  /* `ranked` holds population indices sorted best-first by external fitness. */
  void update(const std::vector<Offspring>&, const std::vector<int>& ranked,
              int gen);

  double sigma() const { return sigma_; }
  const Eigen::VectorXd& mean() const { return mean_; }

 private:
  int dim_, numOffspring_, numParents_;
  Eigen::VectorXd weights_;
  double effectiveParents_, pathCovarianceRate_, pathSigmaRate_, rankOneRate_,
      rankMuRate_, sigmaDamping_, expectedStepNorm_;
  Eigen::VectorXd mean_, pathSigma_, pathCovariance_;
  double sigma_;
  Eigen::MatrixXd covariance_, eigenvectors_;
  std::mt19937 rng_;
  std::normal_distribution<double> randn_;
};

}  // namespace mybem
