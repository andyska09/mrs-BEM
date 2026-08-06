#include "mybem/bem/gsl_helper.h"

#include <cmath>
#include <cstdio>

namespace mybem {

GSLHelper::GSLHelper() {
  gsl_set_error_handler_off();  // report-and-continue; never abort the process
  F.function = &solverV1;
  F.params = (void*)&p;
  T = gsl_root_fsolver_brent;
  s = gsl_root_fsolver_alloc(T);
  p.wR = gsl_integration_workspace_alloc(p.gsl_int_size);
  p.wPsi = gsl_integration_workspace_alloc(p.gsl_int_size);
  p.fPsi = new gsl_function{&integrandPsi, &p};
  p.fR = new gsl_function{&integrandR, &p};
}

GSLHelper::~GSLHelper() {
  gsl_root_fsolver_free(s);
  gsl_integration_workspace_free(p.wPsi);
  gsl_integration_workspace_free(p.wR);
  delete p.fPsi;
  delete p.fR;
}

double GSLHelper::solveInducedVelocity() {
  if (std::abs(p.Omega) < 10) return 0;
  p.type = THRUST;
  v1 = _findZero();

  if (p.vver / v1 >= 0 && p.vver / v1 <= 2) {
    const double k0 = 1;
    const double k1 = -1.125;
    const double k2 = -1.372;
    const double k3 = -1.718;
    const double k4 = -0.655;
    const double vz = -p.vver;
    const double tmp = p.vtot;
    p.vtot = std::sqrt(p.vtot * p.vtot - vz * vz);
    p.vver = 0;

    // Solve again for v1 at vz = 0
    double vh = _findZero();
    const double vzvh = vz / vh;
    const double v2 = vh * (k0 + k1 * vzvh + k2 * vzvh * vzvh +
                            k3 * std::pow(vzvh, 3) + k4 * std::pow(vzvh, 4));
    v1 = std::fmax(v1, v2);
    p.vver = -vz;
    p.vtot = tmp;
  }
  return v1;
}

double GSLHelper::_findZero() {
  int status, iter = 0;
  double lower, upper;
  if (GSL_FN_EVAL(&F, v1min) * GSL_FN_EVAL(&F, v1max) >= 0) {
    v1min = -20;
    v1max = 30;
    const double flo = GSL_FN_EVAL(&F, v1min), fhi = GSL_FN_EVAL(&F, v1max);
    if (flo * fhi >= 0) {
      // Root not bracketed even in the widened window (e.g. extreme params
      // during CMA-ES): clamp to the closest endpoint instead of aborting.
      return std::abs(flo) < std::abs(fhi) ? v1min : v1max;
    }
  }
  gsl_root_fsolver_set(s, &F, v1min, v1max);
  do {
    iter++;
    status = gsl_root_fsolver_iterate(s);
    v1 = gsl_root_fsolver_root(s);
    lower = gsl_root_fsolver_x_lower(s);
    upper = gsl_root_fsolver_x_upper(s);
    status = gsl_root_test_interval(lower, upper, 0, 1e-5);
  } while (status == GSL_CONTINUE && iter < kMaxIter);
  if (status != GSL_SUCCESS) printf("GSL Error occured: %d\n", status);
  return v1;
}

double GSLHelper::integrateThrust() {
  p.type = THRUST;
  return _integrate(&integrandPsi) * p.b * p.rho / (4 * M_PI);
}

double GSLHelper::integrateTorque() {
  p.type = TORQUE;
  return _integrate(&integrandPsi) * p.b * p.rho / (4 * M_PI);
}

double GSLHelper::integrateHForce() {
  p.type = HFORCE;
  return _integrate(&integrandPsi) * p.b * p.rho / (4 * M_PI);
}

double GSLHelper::_integrate(double (*function)(double, void*)) {
  p.fPsi->function = function;
  double result, error;
  gsl_integration_qags(p.fR, 0, p.R, 0, 1e-3, p.gsl_int_size, p.wR, &result,
                       &error);
  return result;
}

void GSLHelper::setPropellerState(double Omega, double vtot, double vver,
                                  double K, double alpha, double mu, double v1,
                                  double a0, double a1s, double b1s) {
  p.Omega = Omega;
  p.vtot = vtot;
  p.K = K;
  p.vver = vver;
  p.alpha = alpha;
  p.mu = mu;
  p.a0 = a0;
  p.a1s = a1s;
  p.b1s = b1s;
  p.v1 = v1;
  p.vhor = std::sqrt(vtot * vtot - vver * vver);
}

}  // namespace mybem
