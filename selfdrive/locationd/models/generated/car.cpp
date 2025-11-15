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
void err_fun(double *nom_x, double *delta_x, double *out_1529549798955663691) {
   out_1529549798955663691[0] = delta_x[0] + nom_x[0];
   out_1529549798955663691[1] = delta_x[1] + nom_x[1];
   out_1529549798955663691[2] = delta_x[2] + nom_x[2];
   out_1529549798955663691[3] = delta_x[3] + nom_x[3];
   out_1529549798955663691[4] = delta_x[4] + nom_x[4];
   out_1529549798955663691[5] = delta_x[5] + nom_x[5];
   out_1529549798955663691[6] = delta_x[6] + nom_x[6];
   out_1529549798955663691[7] = delta_x[7] + nom_x[7];
   out_1529549798955663691[8] = delta_x[8] + nom_x[8];
}
void inv_err_fun(double *nom_x, double *true_x, double *out_1921756172649404678) {
   out_1921756172649404678[0] = -nom_x[0] + true_x[0];
   out_1921756172649404678[1] = -nom_x[1] + true_x[1];
   out_1921756172649404678[2] = -nom_x[2] + true_x[2];
   out_1921756172649404678[3] = -nom_x[3] + true_x[3];
   out_1921756172649404678[4] = -nom_x[4] + true_x[4];
   out_1921756172649404678[5] = -nom_x[5] + true_x[5];
   out_1921756172649404678[6] = -nom_x[6] + true_x[6];
   out_1921756172649404678[7] = -nom_x[7] + true_x[7];
   out_1921756172649404678[8] = -nom_x[8] + true_x[8];
}
void H_mod_fun(double *state, double *out_6794177651193603429) {
   out_6794177651193603429[0] = 1.0;
   out_6794177651193603429[1] = 0.0;
   out_6794177651193603429[2] = 0.0;
   out_6794177651193603429[3] = 0.0;
   out_6794177651193603429[4] = 0.0;
   out_6794177651193603429[5] = 0.0;
   out_6794177651193603429[6] = 0.0;
   out_6794177651193603429[7] = 0.0;
   out_6794177651193603429[8] = 0.0;
   out_6794177651193603429[9] = 0.0;
   out_6794177651193603429[10] = 1.0;
   out_6794177651193603429[11] = 0.0;
   out_6794177651193603429[12] = 0.0;
   out_6794177651193603429[13] = 0.0;
   out_6794177651193603429[14] = 0.0;
   out_6794177651193603429[15] = 0.0;
   out_6794177651193603429[16] = 0.0;
   out_6794177651193603429[17] = 0.0;
   out_6794177651193603429[18] = 0.0;
   out_6794177651193603429[19] = 0.0;
   out_6794177651193603429[20] = 1.0;
   out_6794177651193603429[21] = 0.0;
   out_6794177651193603429[22] = 0.0;
   out_6794177651193603429[23] = 0.0;
   out_6794177651193603429[24] = 0.0;
   out_6794177651193603429[25] = 0.0;
   out_6794177651193603429[26] = 0.0;
   out_6794177651193603429[27] = 0.0;
   out_6794177651193603429[28] = 0.0;
   out_6794177651193603429[29] = 0.0;
   out_6794177651193603429[30] = 1.0;
   out_6794177651193603429[31] = 0.0;
   out_6794177651193603429[32] = 0.0;
   out_6794177651193603429[33] = 0.0;
   out_6794177651193603429[34] = 0.0;
   out_6794177651193603429[35] = 0.0;
   out_6794177651193603429[36] = 0.0;
   out_6794177651193603429[37] = 0.0;
   out_6794177651193603429[38] = 0.0;
   out_6794177651193603429[39] = 0.0;
   out_6794177651193603429[40] = 1.0;
   out_6794177651193603429[41] = 0.0;
   out_6794177651193603429[42] = 0.0;
   out_6794177651193603429[43] = 0.0;
   out_6794177651193603429[44] = 0.0;
   out_6794177651193603429[45] = 0.0;
   out_6794177651193603429[46] = 0.0;
   out_6794177651193603429[47] = 0.0;
   out_6794177651193603429[48] = 0.0;
   out_6794177651193603429[49] = 0.0;
   out_6794177651193603429[50] = 1.0;
   out_6794177651193603429[51] = 0.0;
   out_6794177651193603429[52] = 0.0;
   out_6794177651193603429[53] = 0.0;
   out_6794177651193603429[54] = 0.0;
   out_6794177651193603429[55] = 0.0;
   out_6794177651193603429[56] = 0.0;
   out_6794177651193603429[57] = 0.0;
   out_6794177651193603429[58] = 0.0;
   out_6794177651193603429[59] = 0.0;
   out_6794177651193603429[60] = 1.0;
   out_6794177651193603429[61] = 0.0;
   out_6794177651193603429[62] = 0.0;
   out_6794177651193603429[63] = 0.0;
   out_6794177651193603429[64] = 0.0;
   out_6794177651193603429[65] = 0.0;
   out_6794177651193603429[66] = 0.0;
   out_6794177651193603429[67] = 0.0;
   out_6794177651193603429[68] = 0.0;
   out_6794177651193603429[69] = 0.0;
   out_6794177651193603429[70] = 1.0;
   out_6794177651193603429[71] = 0.0;
   out_6794177651193603429[72] = 0.0;
   out_6794177651193603429[73] = 0.0;
   out_6794177651193603429[74] = 0.0;
   out_6794177651193603429[75] = 0.0;
   out_6794177651193603429[76] = 0.0;
   out_6794177651193603429[77] = 0.0;
   out_6794177651193603429[78] = 0.0;
   out_6794177651193603429[79] = 0.0;
   out_6794177651193603429[80] = 1.0;
}
void f_fun(double *state, double dt, double *out_8563069429564835037) {
   out_8563069429564835037[0] = state[0];
   out_8563069429564835037[1] = state[1];
   out_8563069429564835037[2] = state[2];
   out_8563069429564835037[3] = state[3];
   out_8563069429564835037[4] = state[4];
   out_8563069429564835037[5] = dt*((-state[4] + (-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])/(mass*state[4]))*state[6] - 9.8000000000000007*state[8] + stiffness_front*(-state[2] - state[3] + state[7])*state[0]/(mass*state[1]) + (-stiffness_front*state[0] - stiffness_rear*state[0])*state[5]/(mass*state[4])) + state[5];
   out_8563069429564835037[6] = dt*(center_to_front*stiffness_front*(-state[2] - state[3] + state[7])*state[0]/(rotational_inertia*state[1]) + (-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])*state[5]/(rotational_inertia*state[4]) + (-pow(center_to_front, 2)*stiffness_front*state[0] - pow(center_to_rear, 2)*stiffness_rear*state[0])*state[6]/(rotational_inertia*state[4])) + state[6];
   out_8563069429564835037[7] = state[7];
   out_8563069429564835037[8] = state[8];
}
void F_fun(double *state, double dt, double *out_4848407684010750625) {
   out_4848407684010750625[0] = 1;
   out_4848407684010750625[1] = 0;
   out_4848407684010750625[2] = 0;
   out_4848407684010750625[3] = 0;
   out_4848407684010750625[4] = 0;
   out_4848407684010750625[5] = 0;
   out_4848407684010750625[6] = 0;
   out_4848407684010750625[7] = 0;
   out_4848407684010750625[8] = 0;
   out_4848407684010750625[9] = 0;
   out_4848407684010750625[10] = 1;
   out_4848407684010750625[11] = 0;
   out_4848407684010750625[12] = 0;
   out_4848407684010750625[13] = 0;
   out_4848407684010750625[14] = 0;
   out_4848407684010750625[15] = 0;
   out_4848407684010750625[16] = 0;
   out_4848407684010750625[17] = 0;
   out_4848407684010750625[18] = 0;
   out_4848407684010750625[19] = 0;
   out_4848407684010750625[20] = 1;
   out_4848407684010750625[21] = 0;
   out_4848407684010750625[22] = 0;
   out_4848407684010750625[23] = 0;
   out_4848407684010750625[24] = 0;
   out_4848407684010750625[25] = 0;
   out_4848407684010750625[26] = 0;
   out_4848407684010750625[27] = 0;
   out_4848407684010750625[28] = 0;
   out_4848407684010750625[29] = 0;
   out_4848407684010750625[30] = 1;
   out_4848407684010750625[31] = 0;
   out_4848407684010750625[32] = 0;
   out_4848407684010750625[33] = 0;
   out_4848407684010750625[34] = 0;
   out_4848407684010750625[35] = 0;
   out_4848407684010750625[36] = 0;
   out_4848407684010750625[37] = 0;
   out_4848407684010750625[38] = 0;
   out_4848407684010750625[39] = 0;
   out_4848407684010750625[40] = 1;
   out_4848407684010750625[41] = 0;
   out_4848407684010750625[42] = 0;
   out_4848407684010750625[43] = 0;
   out_4848407684010750625[44] = 0;
   out_4848407684010750625[45] = dt*(stiffness_front*(-state[2] - state[3] + state[7])/(mass*state[1]) + (-stiffness_front - stiffness_rear)*state[5]/(mass*state[4]) + (-center_to_front*stiffness_front + center_to_rear*stiffness_rear)*state[6]/(mass*state[4]));
   out_4848407684010750625[46] = -dt*stiffness_front*(-state[2] - state[3] + state[7])*state[0]/(mass*pow(state[1], 2));
   out_4848407684010750625[47] = -dt*stiffness_front*state[0]/(mass*state[1]);
   out_4848407684010750625[48] = -dt*stiffness_front*state[0]/(mass*state[1]);
   out_4848407684010750625[49] = dt*((-1 - (-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])/(mass*pow(state[4], 2)))*state[6] - (-stiffness_front*state[0] - stiffness_rear*state[0])*state[5]/(mass*pow(state[4], 2)));
   out_4848407684010750625[50] = dt*(-stiffness_front*state[0] - stiffness_rear*state[0])/(mass*state[4]) + 1;
   out_4848407684010750625[51] = dt*(-state[4] + (-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])/(mass*state[4]));
   out_4848407684010750625[52] = dt*stiffness_front*state[0]/(mass*state[1]);
   out_4848407684010750625[53] = -9.8000000000000007*dt;
   out_4848407684010750625[54] = dt*(center_to_front*stiffness_front*(-state[2] - state[3] + state[7])/(rotational_inertia*state[1]) + (-center_to_front*stiffness_front + center_to_rear*stiffness_rear)*state[5]/(rotational_inertia*state[4]) + (-pow(center_to_front, 2)*stiffness_front - pow(center_to_rear, 2)*stiffness_rear)*state[6]/(rotational_inertia*state[4]));
   out_4848407684010750625[55] = -center_to_front*dt*stiffness_front*(-state[2] - state[3] + state[7])*state[0]/(rotational_inertia*pow(state[1], 2));
   out_4848407684010750625[56] = -center_to_front*dt*stiffness_front*state[0]/(rotational_inertia*state[1]);
   out_4848407684010750625[57] = -center_to_front*dt*stiffness_front*state[0]/(rotational_inertia*state[1]);
   out_4848407684010750625[58] = dt*(-(-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])*state[5]/(rotational_inertia*pow(state[4], 2)) - (-pow(center_to_front, 2)*stiffness_front*state[0] - pow(center_to_rear, 2)*stiffness_rear*state[0])*state[6]/(rotational_inertia*pow(state[4], 2)));
   out_4848407684010750625[59] = dt*(-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])/(rotational_inertia*state[4]);
   out_4848407684010750625[60] = dt*(-pow(center_to_front, 2)*stiffness_front*state[0] - pow(center_to_rear, 2)*stiffness_rear*state[0])/(rotational_inertia*state[4]) + 1;
   out_4848407684010750625[61] = center_to_front*dt*stiffness_front*state[0]/(rotational_inertia*state[1]);
   out_4848407684010750625[62] = 0;
   out_4848407684010750625[63] = 0;
   out_4848407684010750625[64] = 0;
   out_4848407684010750625[65] = 0;
   out_4848407684010750625[66] = 0;
   out_4848407684010750625[67] = 0;
   out_4848407684010750625[68] = 0;
   out_4848407684010750625[69] = 0;
   out_4848407684010750625[70] = 1;
   out_4848407684010750625[71] = 0;
   out_4848407684010750625[72] = 0;
   out_4848407684010750625[73] = 0;
   out_4848407684010750625[74] = 0;
   out_4848407684010750625[75] = 0;
   out_4848407684010750625[76] = 0;
   out_4848407684010750625[77] = 0;
   out_4848407684010750625[78] = 0;
   out_4848407684010750625[79] = 0;
   out_4848407684010750625[80] = 1;
}
void h_25(double *state, double *unused, double *out_6300616751131509609) {
   out_6300616751131509609[0] = state[6];
}
void H_25(double *state, double *unused, double *out_3840089371798884113) {
   out_3840089371798884113[0] = 0;
   out_3840089371798884113[1] = 0;
   out_3840089371798884113[2] = 0;
   out_3840089371798884113[3] = 0;
   out_3840089371798884113[4] = 0;
   out_3840089371798884113[5] = 0;
   out_3840089371798884113[6] = 1;
   out_3840089371798884113[7] = 0;
   out_3840089371798884113[8] = 0;
}
void h_24(double *state, double *unused, double *out_4561956304062125783) {
   out_4561956304062125783[0] = state[4];
   out_4561956304062125783[1] = state[5];
}
void H_24(double *state, double *unused, double *out_286728597598211384) {
   out_286728597598211384[0] = 0;
   out_286728597598211384[1] = 0;
   out_286728597598211384[2] = 0;
   out_286728597598211384[3] = 0;
   out_286728597598211384[4] = 1;
   out_286728597598211384[5] = 0;
   out_286728597598211384[6] = 0;
   out_286728597598211384[7] = 0;
   out_286728597598211384[8] = 0;
   out_286728597598211384[9] = 0;
   out_286728597598211384[10] = 0;
   out_286728597598211384[11] = 0;
   out_286728597598211384[12] = 0;
   out_286728597598211384[13] = 0;
   out_286728597598211384[14] = 1;
   out_286728597598211384[15] = 0;
   out_286728597598211384[16] = 0;
   out_286728597598211384[17] = 0;
}
void h_30(double *state, double *unused, double *out_6249656637021483022) {
   out_6249656637021483022[0] = state[4];
}
void H_30(double *state, double *unused, double *out_6358422330306132740) {
   out_6358422330306132740[0] = 0;
   out_6358422330306132740[1] = 0;
   out_6358422330306132740[2] = 0;
   out_6358422330306132740[3] = 0;
   out_6358422330306132740[4] = 1;
   out_6358422330306132740[5] = 0;
   out_6358422330306132740[6] = 0;
   out_6358422330306132740[7] = 0;
   out_6358422330306132740[8] = 0;
}
void h_26(double *state, double *unused, double *out_3645251316212785371) {
   out_3645251316212785371[0] = state[7];
}
void H_26(double *state, double *unused, double *out_98586052924827889) {
   out_98586052924827889[0] = 0;
   out_98586052924827889[1] = 0;
   out_98586052924827889[2] = 0;
   out_98586052924827889[3] = 0;
   out_98586052924827889[4] = 0;
   out_98586052924827889[5] = 0;
   out_98586052924827889[6] = 0;
   out_98586052924827889[7] = 1;
   out_98586052924827889[8] = 0;
}
void h_27(double *state, double *unused, double *out_7468360552207651346) {
   out_7468360552207651346[0] = state[3];
}
void H_27(double *state, double *unused, double *out_4183659018505707829) {
   out_4183659018505707829[0] = 0;
   out_4183659018505707829[1] = 0;
   out_4183659018505707829[2] = 0;
   out_4183659018505707829[3] = 1;
   out_4183659018505707829[4] = 0;
   out_4183659018505707829[5] = 0;
   out_4183659018505707829[6] = 0;
   out_4183659018505707829[7] = 0;
   out_4183659018505707829[8] = 0;
}
void h_29(double *state, double *unused, double *out_7625547220558780491) {
   out_7625547220558780491[0] = state[1];
}
void H_29(double *state, double *unused, double *out_6868653674620524924) {
   out_6868653674620524924[0] = 0;
   out_6868653674620524924[1] = 1;
   out_6868653674620524924[2] = 0;
   out_6868653674620524924[3] = 0;
   out_6868653674620524924[4] = 0;
   out_6868653674620524924[5] = 0;
   out_6868653674620524924[6] = 0;
   out_6868653674620524924[7] = 0;
   out_6868653674620524924[8] = 0;
}
void h_28(double *state, double *unused, double *out_6865044667531173829) {
   out_6865044667531173829[0] = state[0];
}
void H_28(double *state, double *unused, double *out_1786254657550994350) {
   out_1786254657550994350[0] = 1;
   out_1786254657550994350[1] = 0;
   out_1786254657550994350[2] = 0;
   out_1786254657550994350[3] = 0;
   out_1786254657550994350[4] = 0;
   out_1786254657550994350[5] = 0;
   out_1786254657550994350[6] = 0;
   out_1786254657550994350[7] = 0;
   out_1786254657550994350[8] = 0;
}
void h_31(double *state, double *unused, double *out_2339260725587032275) {
   out_2339260725587032275[0] = state[8];
}
void H_31(double *state, double *unused, double *out_527622049308523587) {
   out_527622049308523587[0] = 0;
   out_527622049308523587[1] = 0;
   out_527622049308523587[2] = 0;
   out_527622049308523587[3] = 0;
   out_527622049308523587[4] = 0;
   out_527622049308523587[5] = 0;
   out_527622049308523587[6] = 0;
   out_527622049308523587[7] = 0;
   out_527622049308523587[8] = 1;
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
void car_err_fun(double *nom_x, double *delta_x, double *out_1529549798955663691) {
  err_fun(nom_x, delta_x, out_1529549798955663691);
}
void car_inv_err_fun(double *nom_x, double *true_x, double *out_1921756172649404678) {
  inv_err_fun(nom_x, true_x, out_1921756172649404678);
}
void car_H_mod_fun(double *state, double *out_6794177651193603429) {
  H_mod_fun(state, out_6794177651193603429);
}
void car_f_fun(double *state, double dt, double *out_8563069429564835037) {
  f_fun(state,  dt, out_8563069429564835037);
}
void car_F_fun(double *state, double dt, double *out_4848407684010750625) {
  F_fun(state,  dt, out_4848407684010750625);
}
void car_h_25(double *state, double *unused, double *out_6300616751131509609) {
  h_25(state, unused, out_6300616751131509609);
}
void car_H_25(double *state, double *unused, double *out_3840089371798884113) {
  H_25(state, unused, out_3840089371798884113);
}
void car_h_24(double *state, double *unused, double *out_4561956304062125783) {
  h_24(state, unused, out_4561956304062125783);
}
void car_H_24(double *state, double *unused, double *out_286728597598211384) {
  H_24(state, unused, out_286728597598211384);
}
void car_h_30(double *state, double *unused, double *out_6249656637021483022) {
  h_30(state, unused, out_6249656637021483022);
}
void car_H_30(double *state, double *unused, double *out_6358422330306132740) {
  H_30(state, unused, out_6358422330306132740);
}
void car_h_26(double *state, double *unused, double *out_3645251316212785371) {
  h_26(state, unused, out_3645251316212785371);
}
void car_H_26(double *state, double *unused, double *out_98586052924827889) {
  H_26(state, unused, out_98586052924827889);
}
void car_h_27(double *state, double *unused, double *out_7468360552207651346) {
  h_27(state, unused, out_7468360552207651346);
}
void car_H_27(double *state, double *unused, double *out_4183659018505707829) {
  H_27(state, unused, out_4183659018505707829);
}
void car_h_29(double *state, double *unused, double *out_7625547220558780491) {
  h_29(state, unused, out_7625547220558780491);
}
void car_H_29(double *state, double *unused, double *out_6868653674620524924) {
  H_29(state, unused, out_6868653674620524924);
}
void car_h_28(double *state, double *unused, double *out_6865044667531173829) {
  h_28(state, unused, out_6865044667531173829);
}
void car_H_28(double *state, double *unused, double *out_1786254657550994350) {
  H_28(state, unused, out_1786254657550994350);
}
void car_h_31(double *state, double *unused, double *out_2339260725587032275) {
  h_31(state, unused, out_2339260725587032275);
}
void car_H_31(double *state, double *unused, double *out_527622049308523587) {
  H_31(state, unused, out_527622049308523587);
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
