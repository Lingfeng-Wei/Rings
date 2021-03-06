#include"../src/rings.h"
#include<gsl/gsl_odeiv.h>
#include<gsl/gsl_errno.h>
#include<string.h>

int main() {
  const size_t NBODIES = 2;
  const double epsabs = 1e-8, epsrel = 1e-8;
  central_body bc;
  body bs[NBODIES];
  const double m1 = 1e-6, a1 = 1.0, e1 = 0.05, I1 = 10.0, Omega1 = 0.0, omega1 = 0.0;
  const double m2 = 1e-3, a2 = 6.0, e2 = 0.25, I2 = -5.0, Omega2 = 60.0, omega2 = 145.0;
  gsl_odeiv_evolve *e = gsl_odeiv_evolve_alloc(body_size_to_vector_size(NBODIES));
  gsl_odeiv_control *con = gsl_odeiv_control_secular_new(epsabs);
  gsl_odeiv_step *step = gsl_odeiv_step_alloc(gsl_odeiv_step_rk8pd, body_size_to_vector_size(NBODIES));
  gsl_integration_workspace *ws;
  const size_t nws = 100000;
  const double T = 1e5;
  double h = 0.1;
  double *y;
  int status;
  double t = 0.0;
  double rhs[BODY_VECTOR_SIZE];
  double spin[3] = {0.0, 0.0, 0.0};
  double amdInitial, amdFinal;
  int i;
  int NRETRIES_MAX = 5;

  y = malloc(body_size_to_vector_size(NBODIES)*sizeof(double));
  memset(&bc, 0, sizeof(central_body));

  ws = gsl_integration_workspace_alloc(nws);

  init_body_from_elements(&(bs[0]), m1, a1, e1, I1, Omega1, omega1, spin, 1.0/0.0, 0.0, 0.0, 0.0);
  init_body_from_elements(&(bs[1]), m2, a2, e2, I2, Omega2, omega2, spin, 1.0/0.0, 0.0, 0.0, 0.0);

  amdInitial = body_system_amd(bs, 2);
  
  int nretries = 0;

  do {
    do {
      status = evolve_system(e, con, step, &t, T, &h, &bc, bs, y, NBODIES, epsabs, 0.0);

      if (status != GSL_SUCCESS) { 
        nretries++;
        h = h / 10.0;
        gsl_odeiv_evolve_reset(e);
        gsl_odeiv_step_reset(step);
      } else {
        nretries = 0;
      }
    } while (status != GSL_SUCCESS && nretries <= NRETRIES_MAX);
  } while (status == GSL_SUCCESS && t < T);

  amdFinal = body_system_amd(bs, 2);

  if (t != T) {
    fprintf(stderr, "Did not advance to stop time (t = %g, stop time = %g); GSL return code %d\n",
            t, T, status);
    return 1;
  }

  if (fabs(amdInitial-amdFinal) > 1e-8) {
    fprintf(stderr, "Did not conserve AMD: initially %.15g, finally %.15g\n", amdInitial, amdFinal);
    return 1;
  }

  gsl_odeiv_evolve_free(e);
  gsl_odeiv_control_free(con);
  gsl_odeiv_step_free(step);
  gsl_integration_workspace_free(ws);
  free(y);

  return 0;
}
