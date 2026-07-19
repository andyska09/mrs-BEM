#include "gslHelper.h"

#define THRUST 1
#define TORQUE 2
#define HFORCE 3

/* Contructor for GSL Solver/Integrator Class */
GSLHelper::GSLHelper() {
  gsl_set_error_handler_off();  // report-and-continue; never abort the process
  F.function = &solverV1;
  F.params = (void*)&p;
  T = gsl_root_fsolver_brent;
  s = gsl_root_fsolver_alloc(T);
  p.wR = gsl_integration_workspace_alloc(p.gsl_int_size);
  p.wPsi = gsl_integration_workspace_alloc(p.gsl_int_size);
}

/* Destructor for GSL Solver/Integrator Class */
GSLHelper::~GSLHelper() {
  gsl_root_fsolver_free(s);
  gsl_integration_workspace_free(p.wPsi);
  gsl_integration_workspace_free(p.wR);
}

/* Uses gsl_root_fsolver to solve the implicit equation that determines
 * the induced velocity */
double GSLHelper::solveInducedVelocity() {
  if (std::abs(p.Omega) < 10) return 0;
  p.type = THRUST;
  p.fPsi = new gsl_function;
  p.fPsi->function = &integrandPsi;
  p.fPsi->params = (void*)&p;

  p.fR = new gsl_function;
  p.fR->function = &integrandR;
  p.fR->params = (void*)&p;

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

/* Function that takes care of solving for the induced velocity */
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
  } while (status == GSL_CONTINUE && iter < MAX_ITER);
  if (status != GSL_SUCCESS) printf("GSL Error occured: %d\n", status);
  return v1;
}

/* Internal function that handles GSL integration for the thrust */
double GSLHelper::integrateThrust() {
  p.type = THRUST;
  double T = _integrate(&integrandPsi);
  T *= p.b * p.rho / (4 * M_PI);
  return T;
}

/* Internal function that handles GSL integration for the torque */
double GSLHelper::integrateTorque() {
  p.type = TORQUE;
  double Q = _integrate(&integrandPsi);
  Q *= p.b * p.rho / (4 * M_PI);
  return Q;
}

/* Internal function that handles GSL integration for the hForce */
double GSLHelper::integrateHForce() {
  p.type = HFORCE;
  double H = _integrate(&integrandPsi);
  H *= p.b * p.rho / (4 * M_PI);
  return H;
}

/* Internal function that prepares the memory for numerical integration */
double GSLHelper::_integrate(double (*function)(double, void*)) {
  p.fPsi = new gsl_function;
  p.fPsi->function = function;
  p.fPsi->params = (void*)&p;

  p.fR = new gsl_function;
  p.fR->function = &integrandR;
  p.fR->params = (void*)&p;

  double result, error;
  gsl_integration_qags(p.fR, 0, p.R, 0, 1e-3, p.gsl_int_size, p.wR, &result,
                       &error);
  delete p.fPsi;
  delete p.fR;
  return result;
}

/* Public function that copies the relevant information to internal variables
 * before running the numerical integration/solving */
bool GSLHelper::setPropellerState(const double Omega, const double vtot,
                                  const double vver, const double K,
                                  const double alpha, const double mu,
                                  const double v1, const double a0,
                                  const double a1s, const double b1s) {
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
  return true;
}

/* Function to set the induced velocity once it is known */
void GSLHelper::setv1(const double v1) { p.v1 = v1; }

/* Function that GSL calls which it tries to minimize */
double solverV1(double v1, void* param) {
  double result, error;

  GSL_Parameter_s* p = (GSL_Parameter_s*)param;
  p->v1 = v1;
  gsl_integration_qags(p->fR, 0, p->R, 0, 1e-3, p->gsl_int_size, p->wR, &result,
                       &error);

  const double T_bem = result * p->b * p->rho / (4 * M_PI);
  const double tmp = p->vver - v1;
  const double T_momentum =
      2 * p->rho * p->A * v1 * std::sqrt(p->vhor * p->vhor + tmp * tmp);
  return T_bem - T_momentum;
}

/* Inner function of the double integration for the thrust, torque and hforce
 * The implementation is based on W. Proutys book "Helicopter Stability and
 * Performance" (1990) and the paper "Propeller Thrust and Drag in Forward
 * Flight" (Gill, 2016)
 * */
double integrandPsi(double Psi, void* param) {
  GSL_Parameter_s p = *(GSL_Parameter_s*)param;
  const double sPsi = std::sin(Psi);
  const double cPsi = std::cos(Psi);
  const double beta = p.a0 - p.a1s * cPsi - p.b1s * sPsi;
  const double U_T = p.Omega * (p.r + p.R * p.mu * sPsi);
  const double U_P = p.vver - p.v1 * (1 + p.K * p.r / p.R * cPsi) -
                     p.r * p.Omega * (p.a1s * sPsi + p.b1s * cPsi) -
                     p.vver * beta * cPsi;
  const double phi = std::atan2(U_P, U_T);
  const double alpha = p.theta0 + p.theta1 * p.r / p.R + phi;

#if POLAR == 0
  const double cl = p.cl * alpha;
  const double cd = p.cd;
#elif POLAR == 1
  const double cl = p.cl * (std::sin(alpha) * std::cos(alpha) + p.cl_offset);
  const double cd = p.cd * std::sin(alpha) * std::sin(alpha);
#elif POLAR == 2
  const double tmp1 = std::exp(-p.M * (alpha - p.alpha0));
  const double tmp2 = std::exp(p.M * (alpha + p.alpha0));
  const double sigma = (1 + tmp1 + tmp2) / ((1 + tmp1) * (1 + tmp2));
  const double cl = (1 - sigma) * p.cl1 * alpha +
                    sigma * p.cl2 * std::sin(alpha) * std::cos(alpha);
  const double cd = p.cd * std::sin(alpha) * std::sin(alpha);
#endif

#if CHORD == 0
  const double c = p.c;
#elif CHORD == 1
  const double c = p.ci + p.r / p.R * (p.co - p.ci);
#endif

  const double U2 = U_T * U_T + U_P * U_P;
  const double dL = U2 * cl * c;
  const double dD = U2 * cd * c;

  switch (p.type) {
    case THRUST:
      return dL * std::cos(phi) + dD * std::sin(phi);
    case TORQUE:
      return p.r * (-dL * std::sin(phi) + dD * std::cos(phi));
    case HFORCE:
      return sPsi * (-dL * std::sin(phi) + dD * std::cos(phi));
    default:
      return 0;
  }
}

/* Outer integrand for thrust, torque and hforce */
double integrandR(double r, void* param) {
  GSL_Parameter_s* p = (GSL_Parameter_s*)param;
  p->r = r;
  double result, error;
  gsl_integration_qags(p->fPsi, 0, 2 * M_PI, 0, 1e-3, p->gsl_int_size, p->wPsi,
                       &result, &error);
  return result;
}
