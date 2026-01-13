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
void err_fun(double *nom_x, double *delta_x, double *out_4516286648636903722) {
   out_4516286648636903722[0] = delta_x[0] + nom_x[0];
   out_4516286648636903722[1] = delta_x[1] + nom_x[1];
   out_4516286648636903722[2] = delta_x[2] + nom_x[2];
   out_4516286648636903722[3] = delta_x[3] + nom_x[3];
   out_4516286648636903722[4] = delta_x[4] + nom_x[4];
   out_4516286648636903722[5] = delta_x[5] + nom_x[5];
   out_4516286648636903722[6] = delta_x[6] + nom_x[6];
   out_4516286648636903722[7] = delta_x[7] + nom_x[7];
   out_4516286648636903722[8] = delta_x[8] + nom_x[8];
}
void inv_err_fun(double *nom_x, double *true_x, double *out_28703963116213310) {
   out_28703963116213310[0] = -nom_x[0] + true_x[0];
   out_28703963116213310[1] = -nom_x[1] + true_x[1];
   out_28703963116213310[2] = -nom_x[2] + true_x[2];
   out_28703963116213310[3] = -nom_x[3] + true_x[3];
   out_28703963116213310[4] = -nom_x[4] + true_x[4];
   out_28703963116213310[5] = -nom_x[5] + true_x[5];
   out_28703963116213310[6] = -nom_x[6] + true_x[6];
   out_28703963116213310[7] = -nom_x[7] + true_x[7];
   out_28703963116213310[8] = -nom_x[8] + true_x[8];
}
void H_mod_fun(double *state, double *out_8940306794122284730) {
   out_8940306794122284730[0] = 1.0;
   out_8940306794122284730[1] = 0.0;
   out_8940306794122284730[2] = 0.0;
   out_8940306794122284730[3] = 0.0;
   out_8940306794122284730[4] = 0.0;
   out_8940306794122284730[5] = 0.0;
   out_8940306794122284730[6] = 0.0;
   out_8940306794122284730[7] = 0.0;
   out_8940306794122284730[8] = 0.0;
   out_8940306794122284730[9] = 0.0;
   out_8940306794122284730[10] = 1.0;
   out_8940306794122284730[11] = 0.0;
   out_8940306794122284730[12] = 0.0;
   out_8940306794122284730[13] = 0.0;
   out_8940306794122284730[14] = 0.0;
   out_8940306794122284730[15] = 0.0;
   out_8940306794122284730[16] = 0.0;
   out_8940306794122284730[17] = 0.0;
   out_8940306794122284730[18] = 0.0;
   out_8940306794122284730[19] = 0.0;
   out_8940306794122284730[20] = 1.0;
   out_8940306794122284730[21] = 0.0;
   out_8940306794122284730[22] = 0.0;
   out_8940306794122284730[23] = 0.0;
   out_8940306794122284730[24] = 0.0;
   out_8940306794122284730[25] = 0.0;
   out_8940306794122284730[26] = 0.0;
   out_8940306794122284730[27] = 0.0;
   out_8940306794122284730[28] = 0.0;
   out_8940306794122284730[29] = 0.0;
   out_8940306794122284730[30] = 1.0;
   out_8940306794122284730[31] = 0.0;
   out_8940306794122284730[32] = 0.0;
   out_8940306794122284730[33] = 0.0;
   out_8940306794122284730[34] = 0.0;
   out_8940306794122284730[35] = 0.0;
   out_8940306794122284730[36] = 0.0;
   out_8940306794122284730[37] = 0.0;
   out_8940306794122284730[38] = 0.0;
   out_8940306794122284730[39] = 0.0;
   out_8940306794122284730[40] = 1.0;
   out_8940306794122284730[41] = 0.0;
   out_8940306794122284730[42] = 0.0;
   out_8940306794122284730[43] = 0.0;
   out_8940306794122284730[44] = 0.0;
   out_8940306794122284730[45] = 0.0;
   out_8940306794122284730[46] = 0.0;
   out_8940306794122284730[47] = 0.0;
   out_8940306794122284730[48] = 0.0;
   out_8940306794122284730[49] = 0.0;
   out_8940306794122284730[50] = 1.0;
   out_8940306794122284730[51] = 0.0;
   out_8940306794122284730[52] = 0.0;
   out_8940306794122284730[53] = 0.0;
   out_8940306794122284730[54] = 0.0;
   out_8940306794122284730[55] = 0.0;
   out_8940306794122284730[56] = 0.0;
   out_8940306794122284730[57] = 0.0;
   out_8940306794122284730[58] = 0.0;
   out_8940306794122284730[59] = 0.0;
   out_8940306794122284730[60] = 1.0;
   out_8940306794122284730[61] = 0.0;
   out_8940306794122284730[62] = 0.0;
   out_8940306794122284730[63] = 0.0;
   out_8940306794122284730[64] = 0.0;
   out_8940306794122284730[65] = 0.0;
   out_8940306794122284730[66] = 0.0;
   out_8940306794122284730[67] = 0.0;
   out_8940306794122284730[68] = 0.0;
   out_8940306794122284730[69] = 0.0;
   out_8940306794122284730[70] = 1.0;
   out_8940306794122284730[71] = 0.0;
   out_8940306794122284730[72] = 0.0;
   out_8940306794122284730[73] = 0.0;
   out_8940306794122284730[74] = 0.0;
   out_8940306794122284730[75] = 0.0;
   out_8940306794122284730[76] = 0.0;
   out_8940306794122284730[77] = 0.0;
   out_8940306794122284730[78] = 0.0;
   out_8940306794122284730[79] = 0.0;
   out_8940306794122284730[80] = 1.0;
}
void f_fun(double *state, double dt, double *out_5837366013746318941) {
   out_5837366013746318941[0] = state[0];
   out_5837366013746318941[1] = state[1];
   out_5837366013746318941[2] = state[2];
   out_5837366013746318941[3] = state[3];
   out_5837366013746318941[4] = state[4];
   out_5837366013746318941[5] = dt*((-state[4] + (-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])/(mass*state[4]))*state[6] - 9.8000000000000007*state[8] + stiffness_front*(-state[2] - state[3] + state[7])*state[0]/(mass*state[1]) + (-stiffness_front*state[0] - stiffness_rear*state[0])*state[5]/(mass*state[4])) + state[5];
   out_5837366013746318941[6] = dt*(center_to_front*stiffness_front*(-state[2] - state[3] + state[7])*state[0]/(rotational_inertia*state[1]) + (-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])*state[5]/(rotational_inertia*state[4]) + (-pow(center_to_front, 2)*stiffness_front*state[0] - pow(center_to_rear, 2)*stiffness_rear*state[0])*state[6]/(rotational_inertia*state[4])) + state[6];
   out_5837366013746318941[7] = state[7];
   out_5837366013746318941[8] = state[8];
}
void F_fun(double *state, double dt, double *out_5019779894623636516) {
   out_5019779894623636516[0] = 1;
   out_5019779894623636516[1] = 0;
   out_5019779894623636516[2] = 0;
   out_5019779894623636516[3] = 0;
   out_5019779894623636516[4] = 0;
   out_5019779894623636516[5] = 0;
   out_5019779894623636516[6] = 0;
   out_5019779894623636516[7] = 0;
   out_5019779894623636516[8] = 0;
   out_5019779894623636516[9] = 0;
   out_5019779894623636516[10] = 1;
   out_5019779894623636516[11] = 0;
   out_5019779894623636516[12] = 0;
   out_5019779894623636516[13] = 0;
   out_5019779894623636516[14] = 0;
   out_5019779894623636516[15] = 0;
   out_5019779894623636516[16] = 0;
   out_5019779894623636516[17] = 0;
   out_5019779894623636516[18] = 0;
   out_5019779894623636516[19] = 0;
   out_5019779894623636516[20] = 1;
   out_5019779894623636516[21] = 0;
   out_5019779894623636516[22] = 0;
   out_5019779894623636516[23] = 0;
   out_5019779894623636516[24] = 0;
   out_5019779894623636516[25] = 0;
   out_5019779894623636516[26] = 0;
   out_5019779894623636516[27] = 0;
   out_5019779894623636516[28] = 0;
   out_5019779894623636516[29] = 0;
   out_5019779894623636516[30] = 1;
   out_5019779894623636516[31] = 0;
   out_5019779894623636516[32] = 0;
   out_5019779894623636516[33] = 0;
   out_5019779894623636516[34] = 0;
   out_5019779894623636516[35] = 0;
   out_5019779894623636516[36] = 0;
   out_5019779894623636516[37] = 0;
   out_5019779894623636516[38] = 0;
   out_5019779894623636516[39] = 0;
   out_5019779894623636516[40] = 1;
   out_5019779894623636516[41] = 0;
   out_5019779894623636516[42] = 0;
   out_5019779894623636516[43] = 0;
   out_5019779894623636516[44] = 0;
   out_5019779894623636516[45] = dt*(stiffness_front*(-state[2] - state[3] + state[7])/(mass*state[1]) + (-stiffness_front - stiffness_rear)*state[5]/(mass*state[4]) + (-center_to_front*stiffness_front + center_to_rear*stiffness_rear)*state[6]/(mass*state[4]));
   out_5019779894623636516[46] = -dt*stiffness_front*(-state[2] - state[3] + state[7])*state[0]/(mass*pow(state[1], 2));
   out_5019779894623636516[47] = -dt*stiffness_front*state[0]/(mass*state[1]);
   out_5019779894623636516[48] = -dt*stiffness_front*state[0]/(mass*state[1]);
   out_5019779894623636516[49] = dt*((-1 - (-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])/(mass*pow(state[4], 2)))*state[6] - (-stiffness_front*state[0] - stiffness_rear*state[0])*state[5]/(mass*pow(state[4], 2)));
   out_5019779894623636516[50] = dt*(-stiffness_front*state[0] - stiffness_rear*state[0])/(mass*state[4]) + 1;
   out_5019779894623636516[51] = dt*(-state[4] + (-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])/(mass*state[4]));
   out_5019779894623636516[52] = dt*stiffness_front*state[0]/(mass*state[1]);
   out_5019779894623636516[53] = -9.8000000000000007*dt;
   out_5019779894623636516[54] = dt*(center_to_front*stiffness_front*(-state[2] - state[3] + state[7])/(rotational_inertia*state[1]) + (-center_to_front*stiffness_front + center_to_rear*stiffness_rear)*state[5]/(rotational_inertia*state[4]) + (-pow(center_to_front, 2)*stiffness_front - pow(center_to_rear, 2)*stiffness_rear)*state[6]/(rotational_inertia*state[4]));
   out_5019779894623636516[55] = -center_to_front*dt*stiffness_front*(-state[2] - state[3] + state[7])*state[0]/(rotational_inertia*pow(state[1], 2));
   out_5019779894623636516[56] = -center_to_front*dt*stiffness_front*state[0]/(rotational_inertia*state[1]);
   out_5019779894623636516[57] = -center_to_front*dt*stiffness_front*state[0]/(rotational_inertia*state[1]);
   out_5019779894623636516[58] = dt*(-(-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])*state[5]/(rotational_inertia*pow(state[4], 2)) - (-pow(center_to_front, 2)*stiffness_front*state[0] - pow(center_to_rear, 2)*stiffness_rear*state[0])*state[6]/(rotational_inertia*pow(state[4], 2)));
   out_5019779894623636516[59] = dt*(-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])/(rotational_inertia*state[4]);
   out_5019779894623636516[60] = dt*(-pow(center_to_front, 2)*stiffness_front*state[0] - pow(center_to_rear, 2)*stiffness_rear*state[0])/(rotational_inertia*state[4]) + 1;
   out_5019779894623636516[61] = center_to_front*dt*stiffness_front*state[0]/(rotational_inertia*state[1]);
   out_5019779894623636516[62] = 0;
   out_5019779894623636516[63] = 0;
   out_5019779894623636516[64] = 0;
   out_5019779894623636516[65] = 0;
   out_5019779894623636516[66] = 0;
   out_5019779894623636516[67] = 0;
   out_5019779894623636516[68] = 0;
   out_5019779894623636516[69] = 0;
   out_5019779894623636516[70] = 1;
   out_5019779894623636516[71] = 0;
   out_5019779894623636516[72] = 0;
   out_5019779894623636516[73] = 0;
   out_5019779894623636516[74] = 0;
   out_5019779894623636516[75] = 0;
   out_5019779894623636516[76] = 0;
   out_5019779894623636516[77] = 0;
   out_5019779894623636516[78] = 0;
   out_5019779894623636516[79] = 0;
   out_5019779894623636516[80] = 1;
}
void h_25(double *state, double *unused, double *out_3339146431292074036) {
   out_3339146431292074036[0] = state[6];
}
void H_25(double *state, double *unused, double *out_5417326144132793053) {
   out_5417326144132793053[0] = 0;
   out_5417326144132793053[1] = 0;
   out_5417326144132793053[2] = 0;
   out_5417326144132793053[3] = 0;
   out_5417326144132793053[4] = 0;
   out_5417326144132793053[5] = 0;
   out_5417326144132793053[6] = 1;
   out_5417326144132793053[7] = 0;
   out_5417326144132793053[8] = 0;
}
void h_24(double *state, double *unused, double *out_2648481969623238997) {
   out_2648481969623238997[0] = state[4];
   out_2648481969623238997[1] = state[5];
}
void H_24(double *state, double *unused, double *out_3191618360153924491) {
   out_3191618360153924491[0] = 0;
   out_3191618360153924491[1] = 0;
   out_3191618360153924491[2] = 0;
   out_3191618360153924491[3] = 0;
   out_3191618360153924491[4] = 1;
   out_3191618360153924491[5] = 0;
   out_3191618360153924491[6] = 0;
   out_3191618360153924491[7] = 0;
   out_3191618360153924491[8] = 0;
   out_3191618360153924491[9] = 0;
   out_3191618360153924491[10] = 0;
   out_3191618360153924491[11] = 0;
   out_3191618360153924491[12] = 0;
   out_3191618360153924491[13] = 0;
   out_3191618360153924491[14] = 1;
   out_3191618360153924491[15] = 0;
   out_3191618360153924491[16] = 0;
   out_3191618360153924491[17] = 0;
}
void h_30(double *state, double *unused, double *out_8336762416067126644) {
   out_8336762416067126644[0] = state[4];
}
void H_30(double *state, double *unused, double *out_1499364197358823702) {
   out_1499364197358823702[0] = 0;
   out_1499364197358823702[1] = 0;
   out_1499364197358823702[2] = 0;
   out_1499364197358823702[3] = 0;
   out_1499364197358823702[4] = 1;
   out_1499364197358823702[5] = 0;
   out_1499364197358823702[6] = 0;
   out_1499364197358823702[7] = 0;
   out_1499364197358823702[8] = 0;
}
void h_26(double *state, double *unused, double *out_1544519442435394865) {
   out_1544519442435394865[0] = state[7];
}
void H_26(double *state, double *unused, double *out_9158829463006849277) {
   out_9158829463006849277[0] = 0;
   out_9158829463006849277[1] = 0;
   out_9158829463006849277[2] = 0;
   out_9158829463006849277[3] = 0;
   out_9158829463006849277[4] = 0;
   out_9158829463006849277[5] = 0;
   out_9158829463006849277[6] = 0;
   out_9158829463006849277[7] = 1;
   out_9158829463006849277[8] = 0;
}
void h_27(double *state, double *unused, double *out_5457163032973923104) {
   out_5457163032973923104[0] = state[3];
}
void H_27(double *state, double *unused, double *out_7721428403076458034) {
   out_7721428403076458034[0] = 0;
   out_7721428403076458034[1] = 0;
   out_7721428403076458034[2] = 0;
   out_7721428403076458034[3] = 1;
   out_7721428403076458034[4] = 0;
   out_7721428403076458034[5] = 0;
   out_7721428403076458034[6] = 0;
   out_7721428403076458034[7] = 0;
   out_7721428403076458034[8] = 0;
}
void h_29(double *state, double *unused, double *out_6834133844894968383) {
   out_6834133844894968383[0] = state[1];
}
void H_29(double *state, double *unused, double *out_5036433746961640939) {
   out_5036433746961640939[0] = 0;
   out_5036433746961640939[1] = 1;
   out_5036433746961640939[2] = 0;
   out_5036433746961640939[3] = 0;
   out_5036433746961640939[4] = 0;
   out_5036433746961640939[5] = 0;
   out_5036433746961640939[6] = 0;
   out_5036433746961640939[7] = 0;
   out_5036433746961640939[8] = 0;
}
void h_28(double *state, double *unused, double *out_6284712865824879923) {
   out_6284712865824879923[0] = state[0];
}
void H_28(double *state, double *unused, double *out_3072803475396314688) {
   out_3072803475396314688[0] = 1;
   out_3072803475396314688[1] = 0;
   out_3072803475396314688[2] = 0;
   out_3072803475396314688[3] = 0;
   out_3072803475396314688[4] = 0;
   out_3072803475396314688[5] = 0;
   out_3072803475396314688[6] = 0;
   out_3072803475396314688[7] = 0;
   out_3072803475396314688[8] = 0;
}
void h_31(double *state, double *unused, double *out_3181959762940944891) {
   out_3181959762940944891[0] = state[8];
}
void H_31(double *state, double *unused, double *out_5386680182255832625) {
   out_5386680182255832625[0] = 0;
   out_5386680182255832625[1] = 0;
   out_5386680182255832625[2] = 0;
   out_5386680182255832625[3] = 0;
   out_5386680182255832625[4] = 0;
   out_5386680182255832625[5] = 0;
   out_5386680182255832625[6] = 0;
   out_5386680182255832625[7] = 0;
   out_5386680182255832625[8] = 1;
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
void car_err_fun(double *nom_x, double *delta_x, double *out_4516286648636903722) {
  err_fun(nom_x, delta_x, out_4516286648636903722);
}
void car_inv_err_fun(double *nom_x, double *true_x, double *out_28703963116213310) {
  inv_err_fun(nom_x, true_x, out_28703963116213310);
}
void car_H_mod_fun(double *state, double *out_8940306794122284730) {
  H_mod_fun(state, out_8940306794122284730);
}
void car_f_fun(double *state, double dt, double *out_5837366013746318941) {
  f_fun(state,  dt, out_5837366013746318941);
}
void car_F_fun(double *state, double dt, double *out_5019779894623636516) {
  F_fun(state,  dt, out_5019779894623636516);
}
void car_h_25(double *state, double *unused, double *out_3339146431292074036) {
  h_25(state, unused, out_3339146431292074036);
}
void car_H_25(double *state, double *unused, double *out_5417326144132793053) {
  H_25(state, unused, out_5417326144132793053);
}
void car_h_24(double *state, double *unused, double *out_2648481969623238997) {
  h_24(state, unused, out_2648481969623238997);
}
void car_H_24(double *state, double *unused, double *out_3191618360153924491) {
  H_24(state, unused, out_3191618360153924491);
}
void car_h_30(double *state, double *unused, double *out_8336762416067126644) {
  h_30(state, unused, out_8336762416067126644);
}
void car_H_30(double *state, double *unused, double *out_1499364197358823702) {
  H_30(state, unused, out_1499364197358823702);
}
void car_h_26(double *state, double *unused, double *out_1544519442435394865) {
  h_26(state, unused, out_1544519442435394865);
}
void car_H_26(double *state, double *unused, double *out_9158829463006849277) {
  H_26(state, unused, out_9158829463006849277);
}
void car_h_27(double *state, double *unused, double *out_5457163032973923104) {
  h_27(state, unused, out_5457163032973923104);
}
void car_H_27(double *state, double *unused, double *out_7721428403076458034) {
  H_27(state, unused, out_7721428403076458034);
}
void car_h_29(double *state, double *unused, double *out_6834133844894968383) {
  h_29(state, unused, out_6834133844894968383);
}
void car_H_29(double *state, double *unused, double *out_5036433746961640939) {
  H_29(state, unused, out_5036433746961640939);
}
void car_h_28(double *state, double *unused, double *out_6284712865824879923) {
  h_28(state, unused, out_6284712865824879923);
}
void car_H_28(double *state, double *unused, double *out_3072803475396314688) {
  H_28(state, unused, out_3072803475396314688);
}
void car_h_31(double *state, double *unused, double *out_3181959762940944891) {
  h_31(state, unused, out_3181959762940944891);
}
void car_H_31(double *state, double *unused, double *out_5386680182255832625) {
  H_31(state, unused, out_5386680182255832625);
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
