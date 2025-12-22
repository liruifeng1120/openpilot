#include "car.h"

namespace {
#define DIM 9
#define EDIM 9
#define MEDIM 9
typedef void (*Hfun)(double *, double *, double *);

double mass;

void set_mass(double x){ mass = x;}

double rotational_inertia;

void set_rotational_inertia(double x){ rotational_inertia = x;}

double center_to_front;

void set_center_to_front(double x){ center_to_front = x;}

double center_to_rear;

void set_center_to_rear(double x){ center_to_rear = x;}

double stiffness_front;

void set_stiffness_front(double x){ stiffness_front = x;}

double stiffness_rear;

void set_stiffness_rear(double x){ stiffness_rear = x;}
const static double MAHA_THRESH_25 = 3.8414588206941227;
const static double MAHA_THRESH_24 = 5.991464547107981;
const static double MAHA_THRESH_30 = 3.8414588206941227;
const static double MAHA_THRESH_26 = 3.8414588206941227;
const static double MAHA_THRESH_27 = 3.8414588206941227;
const static double MAHA_THRESH_29 = 3.8414588206941227;
const static double MAHA_THRESH_28 = 3.8414588206941227;
const static double MAHA_THRESH_31 = 3.8414588206941227;

/******************************************************************************
 *                      Code generated with SymPy 1.14.0                      *
 *                                                                            *
 *              See http://www.sympy.org/ for more information.               *
 *                                                                            *
 *                         This file is part of 'ekf'                         *
 ******************************************************************************/
void err_fun(double *nom_x, double *delta_x, double *out_4017150893924662285) {
   out_4017150893924662285[0] = delta_x[0] + nom_x[0];
   out_4017150893924662285[1] = delta_x[1] + nom_x[1];
   out_4017150893924662285[2] = delta_x[2] + nom_x[2];
   out_4017150893924662285[3] = delta_x[3] + nom_x[3];
   out_4017150893924662285[4] = delta_x[4] + nom_x[4];
   out_4017150893924662285[5] = delta_x[5] + nom_x[5];
   out_4017150893924662285[6] = delta_x[6] + nom_x[6];
   out_4017150893924662285[7] = delta_x[7] + nom_x[7];
   out_4017150893924662285[8] = delta_x[8] + nom_x[8];
}
void inv_err_fun(double *nom_x, double *true_x, double *out_1278396702776297538) {
   out_1278396702776297538[0] = -nom_x[0] + true_x[0];
   out_1278396702776297538[1] = -nom_x[1] + true_x[1];
   out_1278396702776297538[2] = -nom_x[2] + true_x[2];
   out_1278396702776297538[3] = -nom_x[3] + true_x[3];
   out_1278396702776297538[4] = -nom_x[4] + true_x[4];
   out_1278396702776297538[5] = -nom_x[5] + true_x[5];
   out_1278396702776297538[6] = -nom_x[6] + true_x[6];
   out_1278396702776297538[7] = -nom_x[7] + true_x[7];
   out_1278396702776297538[8] = -nom_x[8] + true_x[8];
}
void H_mod_fun(double *state, double *out_4207896994963607203) {
   out_4207896994963607203[0] = 1.0;
   out_4207896994963607203[1] = 0.0;
   out_4207896994963607203[2] = 0.0;
   out_4207896994963607203[3] = 0.0;
   out_4207896994963607203[4] = 0.0;
   out_4207896994963607203[5] = 0.0;
   out_4207896994963607203[6] = 0.0;
   out_4207896994963607203[7] = 0.0;
   out_4207896994963607203[8] = 0.0;
   out_4207896994963607203[9] = 0.0;
   out_4207896994963607203[10] = 1.0;
   out_4207896994963607203[11] = 0.0;
   out_4207896994963607203[12] = 0.0;
   out_4207896994963607203[13] = 0.0;
   out_4207896994963607203[14] = 0.0;
   out_4207896994963607203[15] = 0.0;
   out_4207896994963607203[16] = 0.0;
   out_4207896994963607203[17] = 0.0;
   out_4207896994963607203[18] = 0.0;
   out_4207896994963607203[19] = 0.0;
   out_4207896994963607203[20] = 1.0;
   out_4207896994963607203[21] = 0.0;
   out_4207896994963607203[22] = 0.0;
   out_4207896994963607203[23] = 0.0;
   out_4207896994963607203[24] = 0.0;
   out_4207896994963607203[25] = 0.0;
   out_4207896994963607203[26] = 0.0;
   out_4207896994963607203[27] = 0.0;
   out_4207896994963607203[28] = 0.0;
   out_4207896994963607203[29] = 0.0;
   out_4207896994963607203[30] = 1.0;
   out_4207896994963607203[31] = 0.0;
   out_4207896994963607203[32] = 0.0;
   out_4207896994963607203[33] = 0.0;
   out_4207896994963607203[34] = 0.0;
   out_4207896994963607203[35] = 0.0;
   out_4207896994963607203[36] = 0.0;
   out_4207896994963607203[37] = 0.0;
   out_4207896994963607203[38] = 0.0;
   out_4207896994963607203[39] = 0.0;
   out_4207896994963607203[40] = 1.0;
   out_4207896994963607203[41] = 0.0;
   out_4207896994963607203[42] = 0.0;
   out_4207896994963607203[43] = 0.0;
   out_4207896994963607203[44] = 0.0;
   out_4207896994963607203[45] = 0.0;
   out_4207896994963607203[46] = 0.0;
   out_4207896994963607203[47] = 0.0;
   out_4207896994963607203[48] = 0.0;
   out_4207896994963607203[49] = 0.0;
   out_4207896994963607203[50] = 1.0;
   out_4207896994963607203[51] = 0.0;
   out_4207896994963607203[52] = 0.0;
   out_4207896994963607203[53] = 0.0;
   out_4207896994963607203[54] = 0.0;
   out_4207896994963607203[55] = 0.0;
   out_4207896994963607203[56] = 0.0;
   out_4207896994963607203[57] = 0.0;
   out_4207896994963607203[58] = 0.0;
   out_4207896994963607203[59] = 0.0;
   out_4207896994963607203[60] = 1.0;
   out_4207896994963607203[61] = 0.0;
   out_4207896994963607203[62] = 0.0;
   out_4207896994963607203[63] = 0.0;
   out_4207896994963607203[64] = 0.0;
   out_4207896994963607203[65] = 0.0;
   out_4207896994963607203[66] = 0.0;
   out_4207896994963607203[67] = 0.0;
   out_4207896994963607203[68] = 0.0;
   out_4207896994963607203[69] = 0.0;
   out_4207896994963607203[70] = 1.0;
   out_4207896994963607203[71] = 0.0;
   out_4207896994963607203[72] = 0.0;
   out_4207896994963607203[73] = 0.0;
   out_4207896994963607203[74] = 0.0;
   out_4207896994963607203[75] = 0.0;
   out_4207896994963607203[76] = 0.0;
   out_4207896994963607203[77] = 0.0;
   out_4207896994963607203[78] = 0.0;
   out_4207896994963607203[79] = 0.0;
   out_4207896994963607203[80] = 1.0;
}
void f_fun(double *state, double dt, double *out_6269158699136268792) {
   out_6269158699136268792[0] = state[0];
   out_6269158699136268792[1] = state[1];
   out_6269158699136268792[2] = state[2];
   out_6269158699136268792[3] = state[3];
   out_6269158699136268792[4] = state[4];
   out_6269158699136268792[5] = dt*((-state[4] + (-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])/(mass*state[4]))*state[6] - 9.8000000000000007*state[8] + stiffness_front*(-state[2] - state[3] + state[7])*state[0]/(mass*state[1]) + (-stiffness_front*state[0] - stiffness_rear*state[0])*state[5]/(mass*state[4])) + state[5];
   out_6269158699136268792[6] = dt*(center_to_front*stiffness_front*(-state[2] - state[3] + state[7])*state[0]/(rotational_inertia*state[1]) + (-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])*state[5]/(rotational_inertia*state[4]) + (-pow(center_to_front, 2)*stiffness_front*state[0] - pow(center_to_rear, 2)*stiffness_rear*state[0])*state[6]/(rotational_inertia*state[4])) + state[6];
   out_6269158699136268792[7] = state[7];
   out_6269158699136268792[8] = state[8];
}
void F_fun(double *state, double dt, double *out_4428556412686188077) {
   out_4428556412686188077[0] = 1;
   out_4428556412686188077[1] = 0;
   out_4428556412686188077[2] = 0;
   out_4428556412686188077[3] = 0;
   out_4428556412686188077[4] = 0;
   out_4428556412686188077[5] = 0;
   out_4428556412686188077[6] = 0;
   out_4428556412686188077[7] = 0;
   out_4428556412686188077[8] = 0;
   out_4428556412686188077[9] = 0;
   out_4428556412686188077[10] = 1;
   out_4428556412686188077[11] = 0;
   out_4428556412686188077[12] = 0;
   out_4428556412686188077[13] = 0;
   out_4428556412686188077[14] = 0;
   out_4428556412686188077[15] = 0;
   out_4428556412686188077[16] = 0;
   out_4428556412686188077[17] = 0;
   out_4428556412686188077[18] = 0;
   out_4428556412686188077[19] = 0;
   out_4428556412686188077[20] = 1;
   out_4428556412686188077[21] = 0;
   out_4428556412686188077[22] = 0;
   out_4428556412686188077[23] = 0;
   out_4428556412686188077[24] = 0;
   out_4428556412686188077[25] = 0;
   out_4428556412686188077[26] = 0;
   out_4428556412686188077[27] = 0;
   out_4428556412686188077[28] = 0;
   out_4428556412686188077[29] = 0;
   out_4428556412686188077[30] = 1;
   out_4428556412686188077[31] = 0;
   out_4428556412686188077[32] = 0;
   out_4428556412686188077[33] = 0;
   out_4428556412686188077[34] = 0;
   out_4428556412686188077[35] = 0;
   out_4428556412686188077[36] = 0;
   out_4428556412686188077[37] = 0;
   out_4428556412686188077[38] = 0;
   out_4428556412686188077[39] = 0;
   out_4428556412686188077[40] = 1;
   out_4428556412686188077[41] = 0;
   out_4428556412686188077[42] = 0;
   out_4428556412686188077[43] = 0;
   out_4428556412686188077[44] = 0;
   out_4428556412686188077[45] = dt*(stiffness_front*(-state[2] - state[3] + state[7])/(mass*state[1]) + (-stiffness_front - stiffness_rear)*state[5]/(mass*state[4]) + (-center_to_front*stiffness_front + center_to_rear*stiffness_rear)*state[6]/(mass*state[4]));
   out_4428556412686188077[46] = -dt*stiffness_front*(-state[2] - state[3] + state[7])*state[0]/(mass*pow(state[1], 2));
   out_4428556412686188077[47] = -dt*stiffness_front*state[0]/(mass*state[1]);
   out_4428556412686188077[48] = -dt*stiffness_front*state[0]/(mass*state[1]);
   out_4428556412686188077[49] = dt*((-1 - (-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])/(mass*pow(state[4], 2)))*state[6] - (-stiffness_front*state[0] - stiffness_rear*state[0])*state[5]/(mass*pow(state[4], 2)));
   out_4428556412686188077[50] = dt*(-stiffness_front*state[0] - stiffness_rear*state[0])/(mass*state[4]) + 1;
   out_4428556412686188077[51] = dt*(-state[4] + (-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])/(mass*state[4]));
   out_4428556412686188077[52] = dt*stiffness_front*state[0]/(mass*state[1]);
   out_4428556412686188077[53] = -9.8000000000000007*dt;
   out_4428556412686188077[54] = dt*(center_to_front*stiffness_front*(-state[2] - state[3] + state[7])/(rotational_inertia*state[1]) + (-center_to_front*stiffness_front + center_to_rear*stiffness_rear)*state[5]/(rotational_inertia*state[4]) + (-pow(center_to_front, 2)*stiffness_front - pow(center_to_rear, 2)*stiffness_rear)*state[6]/(rotational_inertia*state[4]));
   out_4428556412686188077[55] = -center_to_front*dt*stiffness_front*(-state[2] - state[3] + state[7])*state[0]/(rotational_inertia*pow(state[1], 2));
   out_4428556412686188077[56] = -center_to_front*dt*stiffness_front*state[0]/(rotational_inertia*state[1]);
   out_4428556412686188077[57] = -center_to_front*dt*stiffness_front*state[0]/(rotational_inertia*state[1]);
   out_4428556412686188077[58] = dt*(-(-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])*state[5]/(rotational_inertia*pow(state[4], 2)) - (-pow(center_to_front, 2)*stiffness_front*state[0] - pow(center_to_rear, 2)*stiffness_rear*state[0])*state[6]/(rotational_inertia*pow(state[4], 2)));
   out_4428556412686188077[59] = dt*(-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])/(rotational_inertia*state[4]);
   out_4428556412686188077[60] = dt*(-pow(center_to_front, 2)*stiffness_front*state[0] - pow(center_to_rear, 2)*stiffness_rear*state[0])/(rotational_inertia*state[4]) + 1;
   out_4428556412686188077[61] = center_to_front*dt*stiffness_front*state[0]/(rotational_inertia*state[1]);
   out_4428556412686188077[62] = 0;
   out_4428556412686188077[63] = 0;
   out_4428556412686188077[64] = 0;
   out_4428556412686188077[65] = 0;
   out_4428556412686188077[66] = 0;
   out_4428556412686188077[67] = 0;
   out_4428556412686188077[68] = 0;
   out_4428556412686188077[69] = 0;
   out_4428556412686188077[70] = 1;
   out_4428556412686188077[71] = 0;
   out_4428556412686188077[72] = 0;
   out_4428556412686188077[73] = 0;
   out_4428556412686188077[74] = 0;
   out_4428556412686188077[75] = 0;
   out_4428556412686188077[76] = 0;
   out_4428556412686188077[77] = 0;
   out_4428556412686188077[78] = 0;
   out_4428556412686188077[79] = 0;
   out_4428556412686188077[80] = 1;
}
void h_25(double *state, double *unused, double *out_2015506960498938756) {
   out_2015506960498938756[0] = state[6];
}
void H_25(double *state, double *unused, double *out_3702562552763310491) {
   out_3702562552763310491[0] = 0;
   out_3702562552763310491[1] = 0;
   out_3702562552763310491[2] = 0;
   out_3702562552763310491[3] = 0;
   out_3702562552763310491[4] = 0;
   out_3702562552763310491[5] = 0;
   out_3702562552763310491[6] = 1;
   out_3702562552763310491[7] = 0;
   out_3702562552763310491[8] = 0;
}
void h_24(double *state, double *unused, double *out_8558312059642240844) {
   out_8558312059642240844[0] = state[4];
   out_8558312059642240844[1] = state[5];
}
void H_24(double *state, double *unused, double *out_4173020034806649215) {
   out_4173020034806649215[0] = 0;
   out_4173020034806649215[1] = 0;
   out_4173020034806649215[2] = 0;
   out_4173020034806649215[3] = 0;
   out_4173020034806649215[4] = 1;
   out_4173020034806649215[5] = 0;
   out_4173020034806649215[6] = 0;
   out_4173020034806649215[7] = 0;
   out_4173020034806649215[8] = 0;
   out_4173020034806649215[9] = 0;
   out_4173020034806649215[10] = 0;
   out_4173020034806649215[11] = 0;
   out_4173020034806649215[12] = 0;
   out_4173020034806649215[13] = 0;
   out_4173020034806649215[14] = 1;
   out_4173020034806649215[15] = 0;
   out_4173020034806649215[16] = 0;
   out_4173020034806649215[17] = 0;
}
void h_30(double *state, double *unused, double *out_4139940235341721373) {
   out_4139940235341721373[0] = state[4];
}
void H_30(double *state, double *unused, double *out_3831901499906550561) {
   out_3831901499906550561[0] = 0;
   out_3831901499906550561[1] = 0;
   out_3831901499906550561[2] = 0;
   out_3831901499906550561[3] = 0;
   out_3831901499906550561[4] = 1;
   out_3831901499906550561[5] = 0;
   out_3831901499906550561[6] = 0;
   out_3831901499906550561[7] = 0;
   out_3831901499906550561[8] = 0;
}
void h_26(double *state, double *unused, double *out_861733032319124272) {
   out_861733032319124272[0] = state[7];
}
void H_26(double *state, double *unused, double *out_7444065871637366715) {
   out_7444065871637366715[0] = 0;
   out_7444065871637366715[1] = 0;
   out_7444065871637366715[2] = 0;
   out_7444065871637366715[3] = 0;
   out_7444065871637366715[4] = 0;
   out_7444065871637366715[5] = 0;
   out_7444065871637366715[6] = 0;
   out_7444065871637366715[7] = 1;
   out_7444065871637366715[8] = 0;
}
void h_27(double *state, double *unused, double *out_9220327626956391349) {
   out_9220327626956391349[0] = state[3];
}
void H_27(double *state, double *unused, double *out_6006664811706975472) {
   out_6006664811706975472[0] = 0;
   out_6006664811706975472[1] = 0;
   out_6006664811706975472[2] = 0;
   out_6006664811706975472[3] = 1;
   out_6006664811706975472[4] = 0;
   out_6006664811706975472[5] = 0;
   out_6006664811706975472[6] = 0;
   out_6006664811706975472[7] = 0;
   out_6006664811706975472[8] = 0;
}
void h_29(double *state, double *unused, double *out_2449492400606040413) {
   out_2449492400606040413[0] = state[1];
}
void H_29(double *state, double *unused, double *out_3321670155592158377) {
   out_3321670155592158377[0] = 0;
   out_3321670155592158377[1] = 1;
   out_3321670155592158377[2] = 0;
   out_3321670155592158377[3] = 0;
   out_3321670155592158377[4] = 0;
   out_3321670155592158377[5] = 0;
   out_3321670155592158377[6] = 0;
   out_3321670155592158377[7] = 0;
   out_3321670155592158377[8] = 0;
}
void h_28(double *state, double *unused, double *out_8052583825880249612) {
   out_8052583825880249612[0] = state[0];
}
void H_28(double *state, double *unused, double *out_1358039884026832126) {
   out_1358039884026832126[0] = 1;
   out_1358039884026832126[1] = 0;
   out_1358039884026832126[2] = 0;
   out_1358039884026832126[3] = 0;
   out_1358039884026832126[4] = 0;
   out_1358039884026832126[5] = 0;
   out_1358039884026832126[6] = 0;
   out_1358039884026832126[7] = 0;
   out_1358039884026832126[8] = 0;
}
void h_31(double *state, double *unused, double *out_4755328265851412180) {
   out_4755328265851412180[0] = state[8];
}
void H_31(double *state, double *unused, double *out_3671916590886350063) {
   out_3671916590886350063[0] = 0;
   out_3671916590886350063[1] = 0;
   out_3671916590886350063[2] = 0;
   out_3671916590886350063[3] = 0;
   out_3671916590886350063[4] = 0;
   out_3671916590886350063[5] = 0;
   out_3671916590886350063[6] = 0;
   out_3671916590886350063[7] = 0;
   out_3671916590886350063[8] = 1;
}
#include <eigen3/Eigen/Dense>
#include <iostream>

typedef Eigen::Matrix<double, DIM, DIM, Eigen::RowMajor> DDM;
typedef Eigen::Matrix<double, EDIM, EDIM, Eigen::RowMajor> EEM;
typedef Eigen::Matrix<double, DIM, EDIM, Eigen::RowMajor> DEM;

void predict(double *in_x, double *in_P, double *in_Q, double dt) {
  typedef Eigen::Matrix<double, MEDIM, MEDIM, Eigen::RowMajor> RRM;

  double nx[DIM] = {0};
  double in_F[EDIM*EDIM] = {0};

  // functions from sympy
  f_fun(in_x, dt, nx);
  F_fun(in_x, dt, in_F);


  EEM F(in_F);
  EEM P(in_P);
  EEM Q(in_Q);

  RRM F_main = F.topLeftCorner(MEDIM, MEDIM);
  P.topLeftCorner(MEDIM, MEDIM) = (F_main * P.topLeftCorner(MEDIM, MEDIM)) * F_main.transpose();
  P.topRightCorner(MEDIM, EDIM - MEDIM) = F_main * P.topRightCorner(MEDIM, EDIM - MEDIM);
  P.bottomLeftCorner(EDIM - MEDIM, MEDIM) = P.bottomLeftCorner(EDIM - MEDIM, MEDIM) * F_main.transpose();

  P = P + dt*Q;

  // copy out state
  memcpy(in_x, nx, DIM * sizeof(double));
  memcpy(in_P, P.data(), EDIM * EDIM * sizeof(double));
}

// note: extra_args dim only correct when null space projecting
// otherwise 1
template <int ZDIM, int EADIM, bool MAHA_TEST>
void update(double *in_x, double *in_P, Hfun h_fun, Hfun H_fun, Hfun Hea_fun, double *in_z, double *in_R, double *in_ea, double MAHA_THRESHOLD) {
  typedef Eigen::Matrix<double, ZDIM, ZDIM, Eigen::RowMajor> ZZM;
  typedef Eigen::Matrix<double, ZDIM, DIM, Eigen::RowMajor> ZDM;
  typedef Eigen::Matrix<double, Eigen::Dynamic, EDIM, Eigen::RowMajor> XEM;
  //typedef Eigen::Matrix<double, EDIM, ZDIM, Eigen::RowMajor> EZM;
  typedef Eigen::Matrix<double, Eigen::Dynamic, 1> X1M;
  typedef Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> XXM;

  double in_hx[ZDIM] = {0};
  double in_H[ZDIM * DIM] = {0};
  double in_H_mod[EDIM * DIM] = {0};
  double delta_x[EDIM] = {0};
  double x_new[DIM] = {0};


  // state x, P
  Eigen::Matrix<double, ZDIM, 1> z(in_z);
  EEM P(in_P);
  ZZM pre_R(in_R);

  // functions from sympy
  h_fun(in_x, in_ea, in_hx);
  H_fun(in_x, in_ea, in_H);
  ZDM pre_H(in_H);

  // get y (y = z - hx)
  Eigen::Matrix<double, ZDIM, 1> pre_y(in_hx); pre_y = z - pre_y;
  X1M y; XXM H; XXM R;
  if (Hea_fun){
    typedef Eigen::Matrix<double, ZDIM, EADIM, Eigen::RowMajor> ZAM;
    double in_Hea[ZDIM * EADIM] = {0};
    Hea_fun(in_x, in_ea, in_Hea);
    ZAM Hea(in_Hea);
    XXM A = Hea.transpose().fullPivLu().kernel();


    y = A.transpose() * pre_y;
    H = A.transpose() * pre_H;
    R = A.transpose() * pre_R * A;
  } else {
    y = pre_y;
    H = pre_H;
    R = pre_R;
  }
  // get modified H
  H_mod_fun(in_x, in_H_mod);
  DEM H_mod(in_H_mod);
  XEM H_err = H * H_mod;

  // Do mahalobis distance test
  if (MAHA_TEST){
    XXM a = (H_err * P * H_err.transpose() + R).inverse();
    double maha_dist = y.transpose() * a * y;
    if (maha_dist > MAHA_THRESHOLD){
      R = 1.0e16 * R;
    }
  }

  // Outlier resilient weighting
  double weight = 1;//(1.5)/(1 + y.squaredNorm()/R.sum());

  // kalman gains and I_KH
  XXM S = ((H_err * P) * H_err.transpose()) + R/weight;
  XEM KT = S.fullPivLu().solve(H_err * P.transpose());
  //EZM K = KT.transpose(); TODO: WHY DOES THIS NOT COMPILE?
  //EZM K = S.fullPivLu().solve(H_err * P.transpose()).transpose();
  //std::cout << "Here is the matrix rot:\n" << K << std::endl;
  EEM I_KH = Eigen::Matrix<double, EDIM, EDIM>::Identity() - (KT.transpose() * H_err);

  // update state by injecting dx
  Eigen::Matrix<double, EDIM, 1> dx(delta_x);
  dx  = (KT.transpose() * y);
  memcpy(delta_x, dx.data(), EDIM * sizeof(double));
  err_fun(in_x, delta_x, x_new);
  Eigen::Matrix<double, DIM, 1> x(x_new);

  // update cov
  P = ((I_KH * P) * I_KH.transpose()) + ((KT.transpose() * R) * KT);

  // copy out state
  memcpy(in_x, x.data(), DIM * sizeof(double));
  memcpy(in_P, P.data(), EDIM * EDIM * sizeof(double));
  memcpy(in_z, y.data(), y.rows() * sizeof(double));
}




}
extern "C" {

void car_update_25(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<1, 3, 0>(in_x, in_P, h_25, H_25, NULL, in_z, in_R, in_ea, MAHA_THRESH_25);
}
void car_update_24(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<2, 3, 0>(in_x, in_P, h_24, H_24, NULL, in_z, in_R, in_ea, MAHA_THRESH_24);
}
void car_update_30(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<1, 3, 0>(in_x, in_P, h_30, H_30, NULL, in_z, in_R, in_ea, MAHA_THRESH_30);
}
void car_update_26(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<1, 3, 0>(in_x, in_P, h_26, H_26, NULL, in_z, in_R, in_ea, MAHA_THRESH_26);
}
void car_update_27(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<1, 3, 0>(in_x, in_P, h_27, H_27, NULL, in_z, in_R, in_ea, MAHA_THRESH_27);
}
void car_update_29(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<1, 3, 0>(in_x, in_P, h_29, H_29, NULL, in_z, in_R, in_ea, MAHA_THRESH_29);
}
void car_update_28(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<1, 3, 0>(in_x, in_P, h_28, H_28, NULL, in_z, in_R, in_ea, MAHA_THRESH_28);
}
void car_update_31(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<1, 3, 0>(in_x, in_P, h_31, H_31, NULL, in_z, in_R, in_ea, MAHA_THRESH_31);
}
void car_err_fun(double *nom_x, double *delta_x, double *out_4017150893924662285) {
  err_fun(nom_x, delta_x, out_4017150893924662285);
}
void car_inv_err_fun(double *nom_x, double *true_x, double *out_1278396702776297538) {
  inv_err_fun(nom_x, true_x, out_1278396702776297538);
}
void car_H_mod_fun(double *state, double *out_4207896994963607203) {
  H_mod_fun(state, out_4207896994963607203);
}
void car_f_fun(double *state, double dt, double *out_6269158699136268792) {
  f_fun(state,  dt, out_6269158699136268792);
}
void car_F_fun(double *state, double dt, double *out_4428556412686188077) {
  F_fun(state,  dt, out_4428556412686188077);
}
void car_h_25(double *state, double *unused, double *out_2015506960498938756) {
  h_25(state, unused, out_2015506960498938756);
}
void car_H_25(double *state, double *unused, double *out_3702562552763310491) {
  H_25(state, unused, out_3702562552763310491);
}
void car_h_24(double *state, double *unused, double *out_8558312059642240844) {
  h_24(state, unused, out_8558312059642240844);
}
void car_H_24(double *state, double *unused, double *out_4173020034806649215) {
  H_24(state, unused, out_4173020034806649215);
}
void car_h_30(double *state, double *unused, double *out_4139940235341721373) {
  h_30(state, unused, out_4139940235341721373);
}
void car_H_30(double *state, double *unused, double *out_3831901499906550561) {
  H_30(state, unused, out_3831901499906550561);
}
void car_h_26(double *state, double *unused, double *out_861733032319124272) {
  h_26(state, unused, out_861733032319124272);
}
void car_H_26(double *state, double *unused, double *out_7444065871637366715) {
  H_26(state, unused, out_7444065871637366715);
}
void car_h_27(double *state, double *unused, double *out_9220327626956391349) {
  h_27(state, unused, out_9220327626956391349);
}
void car_H_27(double *state, double *unused, double *out_6006664811706975472) {
  H_27(state, unused, out_6006664811706975472);
}
void car_h_29(double *state, double *unused, double *out_2449492400606040413) {
  h_29(state, unused, out_2449492400606040413);
}
void car_H_29(double *state, double *unused, double *out_3321670155592158377) {
  H_29(state, unused, out_3321670155592158377);
}
void car_h_28(double *state, double *unused, double *out_8052583825880249612) {
  h_28(state, unused, out_8052583825880249612);
}
void car_H_28(double *state, double *unused, double *out_1358039884026832126) {
  H_28(state, unused, out_1358039884026832126);
}
void car_h_31(double *state, double *unused, double *out_4755328265851412180) {
  h_31(state, unused, out_4755328265851412180);
}
void car_H_31(double *state, double *unused, double *out_3671916590886350063) {
  H_31(state, unused, out_3671916590886350063);
}
void car_predict(double *in_x, double *in_P, double *in_Q, double dt) {
  predict(in_x, in_P, in_Q, dt);
}
void car_set_mass(double x) {
  set_mass(x);
}
void car_set_rotational_inertia(double x) {
  set_rotational_inertia(x);
}
void car_set_center_to_front(double x) {
  set_center_to_front(x);
}
void car_set_center_to_rear(double x) {
  set_center_to_rear(x);
}
void car_set_stiffness_front(double x) {
  set_stiffness_front(x);
}
void car_set_stiffness_rear(double x) {
  set_stiffness_rear(x);
}
}

const EKF car = {
  .name = "car",
  .kinds = { 25, 24, 30, 26, 27, 29, 28, 31 },
  .feature_kinds = {  },
  .f_fun = car_f_fun,
  .F_fun = car_F_fun,
  .err_fun = car_err_fun,
  .inv_err_fun = car_inv_err_fun,
  .H_mod_fun = car_H_mod_fun,
  .predict = car_predict,
  .hs = {
    { 25, car_h_25 },
    { 24, car_h_24 },
    { 30, car_h_30 },
    { 26, car_h_26 },
    { 27, car_h_27 },
    { 29, car_h_29 },
    { 28, car_h_28 },
    { 31, car_h_31 },
  },
  .Hs = {
    { 25, car_H_25 },
    { 24, car_H_24 },
    { 30, car_H_30 },
    { 26, car_H_26 },
    { 27, car_H_27 },
    { 29, car_H_29 },
    { 28, car_H_28 },
    { 31, car_H_31 },
  },
  .updates = {
    { 25, car_update_25 },
    { 24, car_update_24 },
    { 30, car_update_30 },
    { 26, car_update_26 },
    { 27, car_update_27 },
    { 29, car_update_29 },
    { 28, car_update_28 },
    { 31, car_update_31 },
  },
  .Hes = {
  },
  .sets = {
    { "mass", car_set_mass },
    { "rotational_inertia", car_set_rotational_inertia },
    { "center_to_front", car_set_center_to_front },
    { "center_to_rear", car_set_center_to_rear },
    { "stiffness_front", car_set_stiffness_front },
    { "stiffness_rear", car_set_stiffness_rear },
  },
  .extra_routines = {
  },
};

ekf_lib_init(car)
