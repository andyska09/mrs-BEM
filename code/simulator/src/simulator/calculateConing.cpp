#include "propeller.h"

double Propeller::_calculateConing() {
  std::vector<double> mu(7, 1), Omega(7, 1);
  const double p = rate[0];
  const double q = rate[1];

  for (size_t i = 1; i < mu.size(); ++i) mu[i] = mu[i - 1] * this->mu;
  for (size_t i = 1; i < Omega.size(); ++i)
    Omega[i] = Omega[i - 1] * this->Omega;

  // USE MAPLE WORKSHEET TO CALCULATE THIS FORMULA
  const double a0 =
      (17860.69768 - 9.085245727e-8 * Omega[3] * mu[1] * q +
       5.075429983e-14 * Omega[5] * mu[2] * vind -
       1.201149295e-15 * Omega[5] * mu[3] * p -
       3.287355978e-15 * Omega[6] * alpha * mu[3] -
       8.012930184e-15 * Omega[6] * alpha * mu[1] +
       1.011494156e-15 * Omega[6] * alpha * mu[5] -
       8.680285403 * Omega[1] * mu[1] * p -
       17.36057081 * Omega[2] * alpha * mu[1] +
       1.517241217e-15 * Omega[5] * mu[5] * p -
       5.958332683e-15 * Omega[5] * mu[1] * p -
       1.561670740e-14 * Omega[5] * mu[4] * vind -
       3.121900413e-12 * Omega[4] * mu[4] + 1.237136043e-13 * Omega[5] * vind -
       3.157428176e-15 * Omega[6] * mu[2] + 6.274197178e-16 * Omega[6] * mu[6] -
       2.031302206e-16 * Omega[6] * mu[4] + 268.0341334 * Omega[1] * vind -
       3.589529587 * Omega[2] * mu[2] - 1.902162223e-15 * Omega[6] +
       8.243768249e-12 * Omega[4] - 4.121166794 * Omega[2]) /
      (-4.377059028e8 - 1.147609987e-7 * Omega[4] * mu[2] +
       7.650732730e-8 * Omega[4] * mu[4] + 1.135865755e-14 * Omega[6] * mu[4] -
       2.999395522e-14 * Omega[6] - 2.020271598e-7 * Omega[4] -
       64.98399099 * Omega[2]);
  return a0;
}
