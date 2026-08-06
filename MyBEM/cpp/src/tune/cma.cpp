#include "mybem/cma.h"

#include <cmath>

namespace mybem {

using Eigen::MatrixXd;
using Eigen::VectorXd;

Cma::Cma(int dim, unsigned seed) : dim_(dim), rng_(seed), randn_(0.0, 1.0) {
  numOffspring_ = 4 + (int)(3 * std::log((double)dim_));
  numParents_ = numOffspring_ / 2;

  weights_.resize(numParents_);
  for (int i = 0; i < numParents_; ++i)
    weights_[i] = std::log(numParents_ + 0.5) - std::log(i + 1.0);
  weights_ /= weights_.sum();
  effectiveParents_ = 1.0 / weights_.squaredNorm();

  pathCovarianceRate_ = (4 + effectiveParents_ / dim_) /
                        (dim_ + 4 + 2 * effectiveParents_ / dim_);
  pathSigmaRate_ = (effectiveParents_ + 2) / (dim_ + effectiveParents_ + 5);
  rankOneRate_ = 2.0 / ((dim_ + 1.3) * (dim_ + 1.3) + effectiveParents_);
  rankMuRate_ = std::min(1 - rankOneRate_,
                         2 * (effectiveParents_ - 2 + 1 / effectiveParents_) /
                             ((dim_ + 2) * (dim_ + 2) + effectiveParents_));
  sigmaDamping_ =
      1 +
      2 * std::max(0.0,
                   std::sqrt((effectiveParents_ - 1.0) / (dim_ + 1)) - 1) +
      pathSigmaRate_;
  expectedStepNorm_ = std::sqrt((double)dim_) *
                      (1 - 1.0 / (4 * dim_) + 1.0 / (21.0 * dim_ * dim_));

  mean_ = VectorXd::Zero(dim_);
  sigma_ = 0.2;
  covariance_ = MatrixXd::Identity(dim_, dim_);
  pathSigma_ = VectorXd::Zero(dim_);
  pathCovariance_ = VectorXd::Zero(dim_);
}

std::vector<Offspring> Cma::sample() {
  Eigen::SelfAdjointEigenSolver<MatrixXd> es(covariance_);
  eigenvectors_ = es.eigenvectors();
  VectorXd axisLengths = es.eigenvalues().cwiseMax(1e-12).cwiseSqrt();

  std::vector<Offspring> population(numOffspring_);
  for (Offspring& o : population) {
    o.normal.resize(dim_);
    for (int i = 0; i < dim_; ++i) o.normal[i] = randn_(rng_);
    o.step = eigenvectors_ * (axisLengths.asDiagonal() * o.normal);
    o.point = mean_ + sigma_ * o.step;
  }
  return population;
}

void Cma::update(const std::vector<Offspring>& population,
                 const std::vector<int>& ranked, int gen) {
  VectorXd meanNormal = VectorXd::Zero(dim_);
  VectorXd meanStep = VectorXd::Zero(dim_);
  mean_.setZero();
  for (int i = 0; i < numParents_; ++i) {
    const Offspring& parent = population[ranked[i]];
    meanNormal += weights_[i] * parent.normal;
    meanStep += weights_[i] * parent.step;
    mean_ += weights_[i] * parent.point;
  }

  pathSigma_ =
      (1 - pathSigmaRate_) * pathSigma_ +
      std::sqrt(pathSigmaRate_ * (2 - pathSigmaRate_) * effectiveParents_) *
          (eigenvectors_ * meanNormal);

  double normalizedPathLength =
      pathSigma_.norm() /
      std::sqrt(1 - std::pow(1 - pathSigmaRate_, 2.0 * (gen + 1))) /
      expectedStepNorm_;
  double heaviside = normalizedPathLength < 1.4 + 2.0 / (dim_ + 1) ? 1.0 : 0.0;

  pathCovariance_ = (1 - pathCovarianceRate_) * pathCovariance_ +
                    heaviside *
                        std::sqrt(pathCovarianceRate_ *
                                  (2 - pathCovarianceRate_) *
                                  effectiveParents_) *
                        meanStep;

  MatrixXd rankMuUpdate = MatrixXd::Zero(dim_, dim_);
  for (int i = 0; i < numParents_; ++i)
    rankMuUpdate += weights_[i] * population[ranked[i]].step *
                    population[ranked[i]].step.transpose();

  double stallCorrection =
      (1 - heaviside) * pathCovarianceRate_ * (2 - pathCovarianceRate_);
  covariance_ = (1 - rankOneRate_ - rankMuRate_) * covariance_ +
                rankOneRate_ * (pathCovariance_ * pathCovariance_.transpose() +
                                stallCorrection * covariance_) +
                rankMuRate_ * rankMuUpdate;
  covariance_ = (covariance_ + covariance_.transpose()) / 2;

  sigma_ *= std::exp((pathSigmaRate_ / sigmaDamping_) *
                     (pathSigma_.norm() / expectedStepNorm_ - 1));
}

}  // namespace mybem
