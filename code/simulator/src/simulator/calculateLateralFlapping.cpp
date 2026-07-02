#include "propeller.h"

double Propeller::_calculateLateralFlapping() {
  std::vector<double> mu(7, 1), Omega(7, 1);
  const double p = rate[0];
  const double q = rate[1];

  for (size_t i = 1; i < mu.size(); ++i) mu[i] = mu[i - 1] * this->mu;
  for (size_t i = 1; i < Omega.size(); ++i)
    Omega[i] = Omega[i - 1] * this->Omega;

  // USE MAPLE WORKSHEET TO CALCULATE THIS FORMULA
  const double b1s =
      3.034482349e-15 * Omega[1] *
      (14.44639117 * Omega[4] * q + 0.6202900305 * Omega[5] * mu[5] +
       1.177725817e17 * mu[1] * vind - 1.208793256 * Omega[5] * mu[3] -
       1.157259683 * Omega[5] * mu[1] + 9.730505319e7 * Omega[2] * q -
       3.482171723e15 * Omega[1] * mu[1] - 3086.424696 * Omega[3] * mu[3] -
       5.169754829e8 * Omega[3] * mu[1] - 6.724278493e8 * Omega[2] * p -
       5.988003267e7 * Omega[2] * mu[2] * q +
       1.500000041 * Omega[4] * mu[4] * p - 8.890086873 * Omega[4] * mu[2] * q -
       7.628130118e15 * Omega[1] * alpha * mu[2] - 4.529202255e15 * p -
       15.43924687 * Omega[4] * mu[3] * vind +
       1.748510208e10 * Omega[2] * mu[1] * vind +
       75.26632912 * Omega[4] * mu[1] * vind +
       0.9999999999 * Omega[5] * alpha * mu[4] -
       4.875000134 * Omega[5] * alpha * mu[2] -
       3.625000097 * Omega[4] * mu[2] * p -
       1.132510062e9 * Omega[3] * alpha * mu[2]) /
      (-4.377059028e8 - 1.147609987e-7 * Omega[4] * mu[2] +
       7.650732720e-8 * Omega[4] * mu[4] + 1.135865756e-14 * Omega[6] * mu[4] -
       2.999395520e-14 * Omega[6] - 2.020271601e-7 * Omega[4] -
       64.98399099 * Omega[2]);
  return b1s;
}
