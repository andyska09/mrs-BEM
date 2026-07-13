#pragma once

#define MAX_ITER 100

#include <cmath>

#include "gsl/gsl_errno.h"
#include "gsl/gsl_integration.h"
#include "gsl/gsl_math.h"
#include "gsl/gsl_roots.h"
#include "params.h"

/* Forward Declarations */
double solverV1(double v1, void* param);
double integrandPsi(double Psi, void* param);
double integrandR(double r, void* param);

/* The GSL_Parameter_s struct is used to pass the propeller state to the GSL
 * functions as a void pointer */
struct GSL_Parameter_s : Propeller_s {
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

  const int gsl_int_size = 1000;
  gsl_integration_workspace* wPsi;
  gsl_integration_workspace* wR;
  gsl_function* fPsi;
  gsl_function* fR;
};

/* The GSLHelper class is used to perform the numerical integration and root
 * finding that the blade element model needs. All velocities are expressed
 * in m/s, all forces in N, all torques in N*m and the coordiante system is
 * front-right-down (as customary in helicopter literature).
 */
class GSLHelper {
 public:
  GSLHelper();
  ~GSLHelper();
  double solveInducedVelocity();
  double integrateThrust();
  double integrateTorque();
  double integrateHForce();
  bool setPropellerState(const double Omega, const double vtot,
                         const double vver, const double K, const double alpha,
                         const double mu, const double v1 = 0,
                         const double a0 = 0, const double a1s = 0,
                         const double b1s = 0);
  void setv1(const double v1);
  GSL_Parameter_s p{};

 private:
  double v1;
  double v1min = -5;
  double v1max = 20;

  const gsl_root_fsolver_type* T;
  gsl_root_fsolver* s;
  gsl_function F;

  double _integrate(double (*function)(double, void*));
  double _findZero();
};
