#pragma once

#include <gsl/gsl_errno.h>
#include <gsl/gsl_integration.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_roots.h>

#include "mybem/types.h"

namespace mybem {

constexpr int kMaxIter = 100;

/* Blade-element parameters. Which fields are read depends on the polar and
 * chord function selected at load. */
struct BEMParams {
  double rho = 1.204;
  double R = 5.1 * 2.54 / 2 * 1e-2;
  double b = 3;
  double A = M_PI * (5.1 * 2.54 / 2 * 1e-2) * (5.1 * 2.54 / 2 * 1e-2);
  double theta0 = toRad(21.77);
  double theta1 = toRad(-11);
  double ci = 1.7e-2;
  double co = 0.7e-2;
  double c = 1.3e-2;

  double cl = 15.24214;
  double cd = 13.54894;
  double cl_offset = 0;

  double cl1 = 8.529715;
  double cl2 = 1.357441;
  double Mstall = 20;
  double alpha0 = toRad(12);
};

enum IntegrandType { THRUST = 1, TORQUE = 2, HFORCE = 3 };

struct GSLParams;
using PolarFn = void (*)(const GSLParams&, double alpha, double& cl,
                         double& cd);
using ChordFn = double (*)(const GSLParams&);

void polarSinCos(const GSLParams&, double alpha, double& cl, double& cd);
void polarStall(const GSLParams&, double alpha, double& cl, double& cd);
double chordConstant(const GSLParams&);
double chordLinear(const GSLParams&);

/* Passed to GSL as a void*. Polar and chord are function pointers, not virtual
 * calls: this is the innermost loop of a nested adaptive quadrature. */
struct GSLParams : BEMParams {
  double Omega = 0;
  double vtot = 0;
  double vver = 0;
  double vhor = 0;
  double alpha = 0;
  double mu = 0;
  double a0 = 0;
  double a1s = 0;
  double b1s = 0;
  double r = 0;
  double v1 = 0;
  double K = 0;
  char type = 0;

  PolarFn polar = &polarSinCos;
  ChordFn chord = &chordLinear;

  const int gsl_int_size = 1000;
  gsl_integration_workspace* wPsi = nullptr;
  gsl_integration_workspace* wR = nullptr;
  gsl_function* fPsi = nullptr;
  gsl_function* fR = nullptr;
};

double solverV1(double v1, void* param);
double integrandPsi(double Psi, void* param);
double integrandR(double r, void* param);

/* Numerical integration and root finding for the blade element model. FRD,
 * SI units. */
class GSLHelper {
 public:
  GSLHelper();
  ~GSLHelper();
  GSLHelper(const GSLHelper&) = delete;
  GSLHelper& operator=(const GSLHelper&) = delete;

  double solveInducedVelocity();
  double integrateThrust();
  double integrateTorque();
  double integrateHForce();
  void setPropellerState(double Omega, double vtot, double vver, double K,
                         double alpha, double mu, double v1 = 0, double a0 = 0,
                         double a1s = 0, double b1s = 0);
  void setv1(double v1) { p.v1 = v1; }

  GSLParams p{};

 private:
  double v1 = 0;
  double v1min = -5;
  double v1max = 20;

  const gsl_root_fsolver_type* T;
  gsl_root_fsolver* s;
  gsl_function F;

  double _integrate(double (*function)(double, void*));
  double _findZero();
};

}  // namespace mybem
