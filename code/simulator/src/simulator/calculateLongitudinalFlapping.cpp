#include "propeller.h"

double Propeller::_calculateLongitudinalFlapping() {
  std::vector<double> mu(7, 1), Omega(7, 1);
  const double p = rate[0];
  const double q = rate[1];

  for (size_t i = 1; i < mu.size(); ++i) mu[i] = mu[i - 1] * this->mu;
  for (size_t i = 1; i < Omega.size(); ++i)
    Omega[i] = Omega[i - 1] * this->Omega;

  // USE MAPLE WORKSHEET TO CALCULATE THIS FORMULA
  const double a1s =
      4.543463022e-14 *
      (0.4564908655 * Omega[5] * mu[3] + 0.7417976561 * Omega[5] * mu[1] +
       4.491002632e7 * Omega[2] * q + 0.9648437489 * Omega[4] * p +
       1.559168274e10 * Omega[1] * mu[1] - 58779.37391 * Omega[3] * mu[3] +
       1.398833618e6 * Omega[3] * mu[1] + 6.498797591e6 * Omega[2] * p +
       3.024957887e14 * q - 15.43924657 * Omega[4] * mu[3] * vind -
       3.578285830e6 * Omega[2] * mu[2] * p +
       6.499534432e7 * Omega[2] * mu[1] * vind -
       25.08877564 * Omega[4] * mu[1] * vind -
       1.039925259e8 * Omega[2] * mu[3] * vind + Omega[5] * alpha * mu[4] +
       1.625000001 * Omega[5] * alpha * mu[2] +
       0.5937500002 * Omega[4] * mu[2] * p +
       6.735595891e6 * Omega[3] * alpha * mu[4] -
       4.209748462e6 * Omega[3] * alpha * mu[2]) *
      Omega[1] /
      (-4.377059028e8 - 1.147609987e-7 * Omega[4] * mu[2] +
       7.650732720e-8 * Omega[4] * mu[4] + 1.135865755e-14 * Omega[6] * mu[4] -
       2.999395520e-14 * Omega[6] - 2.020271610e-7 * Omega[4] -
       64.98399099 * Omega[2]);
  return a1s;
}
