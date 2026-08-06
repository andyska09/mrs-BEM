#include <cmath>

#include "mybem/bem/gsl_helper.h"

namespace mybem {

void polarSinCos(const GSLParams& p, double alpha, double& cl, double& cd) {
  cl = p.cl * (std::sin(alpha) * std::cos(alpha) + p.cl_offset);
  cd = p.cd * std::sin(alpha) * std::sin(alpha);
}

void polarStall(const GSLParams& p, double alpha, double& cl, double& cd) {
  const double tmp1 = std::exp(-p.Mstall * (alpha - p.alpha0));
  const double tmp2 = std::exp(p.Mstall * (alpha + p.alpha0));
  const double sigma = (1 + tmp1 + tmp2) / ((1 + tmp1) * (1 + tmp2));
  cl = (1 - sigma) * p.cl1 * alpha +
       sigma * p.cl2 * std::sin(alpha) * std::cos(alpha);
  cd = p.cd * std::sin(alpha) * std::sin(alpha);
}

double chordConstant(const GSLParams& p) { return p.c; }

double chordLinear(const GSLParams& p) {
  return p.ci + p.r / p.R * (p.co - p.ci);
}

/* Inner integrand over the azimuth angle. Prouty, "Helicopter Stability and
 * Performance" (1990); Gill, "Propeller Thrust and Drag in Forward Flight"
 * (2016). */
double integrandPsi(double Psi, void* param) {
  const GSLParams& p = *(const GSLParams*)param;
  const double sPsi = std::sin(Psi);
  const double cPsi = std::cos(Psi);
  const double beta = p.a0 - p.a1s * cPsi - p.b1s * sPsi;
  const double U_T = p.Omega * (p.r + p.R * p.mu * sPsi);
  const double U_P = p.vver - p.v1 -
                     p.r * p.Omega * (p.a1s * sPsi + p.b1s * cPsi) -
                     p.vver * beta * cPsi;
  const double phi = std::atan2(U_P, U_T);
  const double alpha = p.theta0 + p.theta1 * p.r / p.R + phi;

  double cl, cd;
  p.polar(p, alpha, cl, cd);
  const double c = p.chord(p);

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

/* Outer integrand over the blade radius. */
double integrandR(double r, void* param) {
  GSLParams* p = (GSLParams*)param;
  p->r = r;
  double result, error;
  gsl_integration_qags(p->fPsi, 0, 2 * M_PI, 0, 1e-3, p->gsl_int_size, p->wPsi,
                       &result, &error);
  return result;
}

/* Blade-element minus momentum-theory thrust; its root is the induced
 * velocity. */
double solverV1(double v1, void* param) {
  double result, error;

  GSLParams* p = (GSLParams*)param;
  p->v1 = v1;
  gsl_integration_qags(p->fR, 0, p->R, 0, 1e-3, p->gsl_int_size, p->wR, &result,
                       &error);

  const double T_bem = result * p->b * p->rho / (4 * M_PI);
  const double tmp = p->vver - v1;
  const double T_momentum =
      2 * p->rho * p->A * v1 * std::sqrt(p->vhor * p->vhor + tmp * tmp);
  return T_bem - T_momentum;
}

}  // namespace mybem
