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
 *                       Code generated with SymPy 1.12                       *
 *                                                                            *
 *              See http://www.sympy.org/ for more information.               *
 *                                                                            *
 *                         This file is part of 'ekf'                         *
 ******************************************************************************/
void err_fun(double *nom_x, double *delta_x, double *out_8491087941299164185) {
   out_8491087941299164185[0] = delta_x[0] + nom_x[0];
   out_8491087941299164185[1] = delta_x[1] + nom_x[1];
   out_8491087941299164185[2] = delta_x[2] + nom_x[2];
   out_8491087941299164185[3] = delta_x[3] + nom_x[3];
   out_8491087941299164185[4] = delta_x[4] + nom_x[4];
   out_8491087941299164185[5] = delta_x[5] + nom_x[5];
   out_8491087941299164185[6] = delta_x[6] + nom_x[6];
   out_8491087941299164185[7] = delta_x[7] + nom_x[7];
   out_8491087941299164185[8] = delta_x[8] + nom_x[8];
}
void inv_err_fun(double *nom_x, double *true_x, double *out_795413330377817649) {
   out_795413330377817649[0] = -nom_x[0] + true_x[0];
   out_795413330377817649[1] = -nom_x[1] + true_x[1];
   out_795413330377817649[2] = -nom_x[2] + true_x[2];
   out_795413330377817649[3] = -nom_x[3] + true_x[3];
   out_795413330377817649[4] = -nom_x[4] + true_x[4];
   out_795413330377817649[5] = -nom_x[5] + true_x[5];
   out_795413330377817649[6] = -nom_x[6] + true_x[6];
   out_795413330377817649[7] = -nom_x[7] + true_x[7];
   out_795413330377817649[8] = -nom_x[8] + true_x[8];
}
void H_mod_fun(double *state, double *out_7465730014062278352) {
   out_7465730014062278352[0] = 1.0;
   out_7465730014062278352[1] = 0;
   out_7465730014062278352[2] = 0;
   out_7465730014062278352[3] = 0;
   out_7465730014062278352[4] = 0;
   out_7465730014062278352[5] = 0;
   out_7465730014062278352[6] = 0;
   out_7465730014062278352[7] = 0;
   out_7465730014062278352[8] = 0;
   out_7465730014062278352[9] = 0;
   out_7465730014062278352[10] = 1.0;
   out_7465730014062278352[11] = 0;
   out_7465730014062278352[12] = 0;
   out_7465730014062278352[13] = 0;
   out_7465730014062278352[14] = 0;
   out_7465730014062278352[15] = 0;
   out_7465730014062278352[16] = 0;
   out_7465730014062278352[17] = 0;
   out_7465730014062278352[18] = 0;
   out_7465730014062278352[19] = 0;
   out_7465730014062278352[20] = 1.0;
   out_7465730014062278352[21] = 0;
   out_7465730014062278352[22] = 0;
   out_7465730014062278352[23] = 0;
   out_7465730014062278352[24] = 0;
   out_7465730014062278352[25] = 0;
   out_7465730014062278352[26] = 0;
   out_7465730014062278352[27] = 0;
   out_7465730014062278352[28] = 0;
   out_7465730014062278352[29] = 0;
   out_7465730014062278352[30] = 1.0;
   out_7465730014062278352[31] = 0;
   out_7465730014062278352[32] = 0;
   out_7465730014062278352[33] = 0;
   out_7465730014062278352[34] = 0;
   out_7465730014062278352[35] = 0;
   out_7465730014062278352[36] = 0;
   out_7465730014062278352[37] = 0;
   out_7465730014062278352[38] = 0;
   out_7465730014062278352[39] = 0;
   out_7465730014062278352[40] = 1.0;
   out_7465730014062278352[41] = 0;
   out_7465730014062278352[42] = 0;
   out_7465730014062278352[43] = 0;
   out_7465730014062278352[44] = 0;
   out_7465730014062278352[45] = 0;
   out_7465730014062278352[46] = 0;
   out_7465730014062278352[47] = 0;
   out_7465730014062278352[48] = 0;
   out_7465730014062278352[49] = 0;
   out_7465730014062278352[50] = 1.0;
   out_7465730014062278352[51] = 0;
   out_7465730014062278352[52] = 0;
   out_7465730014062278352[53] = 0;
   out_7465730014062278352[54] = 0;
   out_7465730014062278352[55] = 0;
   out_7465730014062278352[56] = 0;
   out_7465730014062278352[57] = 0;
   out_7465730014062278352[58] = 0;
   out_7465730014062278352[59] = 0;
   out_7465730014062278352[60] = 1.0;
   out_7465730014062278352[61] = 0;
   out_7465730014062278352[62] = 0;
   out_7465730014062278352[63] = 0;
   out_7465730014062278352[64] = 0;
   out_7465730014062278352[65] = 0;
   out_7465730014062278352[66] = 0;
   out_7465730014062278352[67] = 0;
   out_7465730014062278352[68] = 0;
   out_7465730014062278352[69] = 0;
   out_7465730014062278352[70] = 1.0;
   out_7465730014062278352[71] = 0;
   out_7465730014062278352[72] = 0;
   out_7465730014062278352[73] = 0;
   out_7465730014062278352[74] = 0;
   out_7465730014062278352[75] = 0;
   out_7465730014062278352[76] = 0;
   out_7465730014062278352[77] = 0;
   out_7465730014062278352[78] = 0;
   out_7465730014062278352[79] = 0;
   out_7465730014062278352[80] = 1.0;
}
void f_fun(double *state, double dt, double *out_7440235772468318890) {
   out_7440235772468318890[0] = state[0];
   out_7440235772468318890[1] = state[1];
   out_7440235772468318890[2] = state[2];
   out_7440235772468318890[3] = state[3];
   out_7440235772468318890[4] = state[4];
   out_7440235772468318890[5] = dt*((-state[4] + (-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])/(mass*state[4]))*state[6] - 9.8000000000000007*state[8] + stiffness_front*(-state[2] - state[3] + state[7])*state[0]/(mass*state[1]) + (-stiffness_front*state[0] - stiffness_rear*state[0])*state[5]/(mass*state[4])) + state[5];
   out_7440235772468318890[6] = dt*(center_to_front*stiffness_front*(-state[2] - state[3] + state[7])*state[0]/(rotational_inertia*state[1]) + (-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])*state[5]/(rotational_inertia*state[4]) + (-pow(center_to_front, 2)*stiffness_front*state[0] - pow(center_to_rear, 2)*stiffness_rear*state[0])*state[6]/(rotational_inertia*state[4])) + state[6];
   out_7440235772468318890[7] = state[7];
   out_7440235772468318890[8] = state[8];
}
void F_fun(double *state, double dt, double *out_2286988896813116968) {
   out_2286988896813116968[0] = 1;
   out_2286988896813116968[1] = 0;
   out_2286988896813116968[2] = 0;
   out_2286988896813116968[3] = 0;
   out_2286988896813116968[4] = 0;
   out_2286988896813116968[5] = 0;
   out_2286988896813116968[6] = 0;
   out_2286988896813116968[7] = 0;
   out_2286988896813116968[8] = 0;
   out_2286988896813116968[9] = 0;
   out_2286988896813116968[10] = 1;
   out_2286988896813116968[11] = 0;
   out_2286988896813116968[12] = 0;
   out_2286988896813116968[13] = 0;
   out_2286988896813116968[14] = 0;
   out_2286988896813116968[15] = 0;
   out_2286988896813116968[16] = 0;
   out_2286988896813116968[17] = 0;
   out_2286988896813116968[18] = 0;
   out_2286988896813116968[19] = 0;
   out_2286988896813116968[20] = 1;
   out_2286988896813116968[21] = 0;
   out_2286988896813116968[22] = 0;
   out_2286988896813116968[23] = 0;
   out_2286988896813116968[24] = 0;
   out_2286988896813116968[25] = 0;
   out_2286988896813116968[26] = 0;
   out_2286988896813116968[27] = 0;
   out_2286988896813116968[28] = 0;
   out_2286988896813116968[29] = 0;
   out_2286988896813116968[30] = 1;
   out_2286988896813116968[31] = 0;
   out_2286988896813116968[32] = 0;
   out_2286988896813116968[33] = 0;
   out_2286988896813116968[34] = 0;
   out_2286988896813116968[35] = 0;
   out_2286988896813116968[36] = 0;
   out_2286988896813116968[37] = 0;
   out_2286988896813116968[38] = 0;
   out_2286988896813116968[39] = 0;
   out_2286988896813116968[40] = 1;
   out_2286988896813116968[41] = 0;
   out_2286988896813116968[42] = 0;
   out_2286988896813116968[43] = 0;
   out_2286988896813116968[44] = 0;
   out_2286988896813116968[45] = dt*(stiffness_front*(-state[2] - state[3] + state[7])/(mass*state[1]) + (-stiffness_front - stiffness_rear)*state[5]/(mass*state[4]) + (-center_to_front*stiffness_front + center_to_rear*stiffness_rear)*state[6]/(mass*state[4]));
   out_2286988896813116968[46] = -dt*stiffness_front*(-state[2] - state[3] + state[7])*state[0]/(mass*pow(state[1], 2));
   out_2286988896813116968[47] = -dt*stiffness_front*state[0]/(mass*state[1]);
   out_2286988896813116968[48] = -dt*stiffness_front*state[0]/(mass*state[1]);
   out_2286988896813116968[49] = dt*((-1 - (-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])/(mass*pow(state[4], 2)))*state[6] - (-stiffness_front*state[0] - stiffness_rear*state[0])*state[5]/(mass*pow(state[4], 2)));
   out_2286988896813116968[50] = dt*(-stiffness_front*state[0] - stiffness_rear*state[0])/(mass*state[4]) + 1;
   out_2286988896813116968[51] = dt*(-state[4] + (-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])/(mass*state[4]));
   out_2286988896813116968[52] = dt*stiffness_front*state[0]/(mass*state[1]);
   out_2286988896813116968[53] = -9.8000000000000007*dt;
   out_2286988896813116968[54] = dt*(center_to_front*stiffness_front*(-state[2] - state[3] + state[7])/(rotational_inertia*state[1]) + (-center_to_front*stiffness_front + center_to_rear*stiffness_rear)*state[5]/(rotational_inertia*state[4]) + (-pow(center_to_front, 2)*stiffness_front - pow(center_to_rear, 2)*stiffness_rear)*state[6]/(rotational_inertia*state[4]));
   out_2286988896813116968[55] = -center_to_front*dt*stiffness_front*(-state[2] - state[3] + state[7])*state[0]/(rotational_inertia*pow(state[1], 2));
   out_2286988896813116968[56] = -center_to_front*dt*stiffness_front*state[0]/(rotational_inertia*state[1]);
   out_2286988896813116968[57] = -center_to_front*dt*stiffness_front*state[0]/(rotational_inertia*state[1]);
   out_2286988896813116968[58] = dt*(-(-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])*state[5]/(rotational_inertia*pow(state[4], 2)) - (-pow(center_to_front, 2)*stiffness_front*state[0] - pow(center_to_rear, 2)*stiffness_rear*state[0])*state[6]/(rotational_inertia*pow(state[4], 2)));
   out_2286988896813116968[59] = dt*(-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])/(rotational_inertia*state[4]);
   out_2286988896813116968[60] = dt*(-pow(center_to_front, 2)*stiffness_front*state[0] - pow(center_to_rear, 2)*stiffness_rear*state[0])/(rotational_inertia*state[4]) + 1;
   out_2286988896813116968[61] = center_to_front*dt*stiffness_front*state[0]/(rotational_inertia*state[1]);
   out_2286988896813116968[62] = 0;
   out_2286988896813116968[63] = 0;
   out_2286988896813116968[64] = 0;
   out_2286988896813116968[65] = 0;
   out_2286988896813116968[66] = 0;
   out_2286988896813116968[67] = 0;
   out_2286988896813116968[68] = 0;
   out_2286988896813116968[69] = 0;
   out_2286988896813116968[70] = 1;
   out_2286988896813116968[71] = 0;
   out_2286988896813116968[72] = 0;
   out_2286988896813116968[73] = 0;
   out_2286988896813116968[74] = 0;
   out_2286988896813116968[75] = 0;
   out_2286988896813116968[76] = 0;
   out_2286988896813116968[77] = 0;
   out_2286988896813116968[78] = 0;
   out_2286988896813116968[79] = 0;
   out_2286988896813116968[80] = 1;
}
void h_25(double *state, double *unused, double *out_6851332156974240934) {
   out_6851332156974240934[0] = state[6];
}
void H_25(double *state, double *unused, double *out_4355595313115228839) {
   out_4355595313115228839[0] = 0;
   out_4355595313115228839[1] = 0;
   out_4355595313115228839[2] = 0;
   out_4355595313115228839[3] = 0;
   out_4355595313115228839[4] = 0;
   out_4355595313115228839[5] = 0;
   out_4355595313115228839[6] = 1;
   out_4355595313115228839[7] = 0;
   out_4355595313115228839[8] = 0;
}
void h_24(double *state, double *unused, double *out_579617225417493254) {
   out_579617225417493254[0] = state[4];
   out_579617225417493254[1] = state[5];
}
void H_24(double *state, double *unused, double *out_4826052795158567563) {
   out_4826052795158567563[0] = 0;
   out_4826052795158567563[1] = 0;
   out_4826052795158567563[2] = 0;
   out_4826052795158567563[3] = 0;
   out_4826052795158567563[4] = 1;
   out_4826052795158567563[5] = 0;
   out_4826052795158567563[6] = 0;
   out_4826052795158567563[7] = 0;
   out_4826052795158567563[8] = 0;
   out_4826052795158567563[9] = 0;
   out_4826052795158567563[10] = 0;
   out_4826052795158567563[11] = 0;
   out_4826052795158567563[12] = 0;
   out_4826052795158567563[13] = 0;
   out_4826052795158567563[14] = 1;
   out_4826052795158567563[15] = 0;
   out_4826052795158567563[16] = 0;
   out_4826052795158567563[17] = 0;
}
void h_30(double *state, double *unused, double *out_2731300491842289496) {
   out_2731300491842289496[0] = state[4];
}
void H_30(double *state, double *unused, double *out_8883291643242837037) {
   out_8883291643242837037[0] = 0;
   out_8883291643242837037[1] = 0;
   out_8883291643242837037[2] = 0;
   out_8883291643242837037[3] = 0;
   out_8883291643242837037[4] = 1;
   out_8883291643242837037[5] = 0;
   out_8883291643242837037[6] = 0;
   out_8883291643242837037[7] = 0;
   out_8883291643242837037[8] = 0;
}
void h_26(double *state, double *unused, double *out_5768086543286622181) {
   out_5768086543286622181[0] = state[7];
}
void H_26(double *state, double *unused, double *out_8097098631989285063) {
   out_8097098631989285063[0] = 0;
   out_8097098631989285063[1] = 0;
   out_8097098631989285063[2] = 0;
   out_8097098631989285063[3] = 0;
   out_8097098631989285063[4] = 0;
   out_8097098631989285063[5] = 0;
   out_8097098631989285063[6] = 0;
   out_8097098631989285063[7] = 1;
   out_8097098631989285063[8] = 0;
}
void h_27(double *state, double *unused, double *out_4592649212942244903) {
   out_4592649212942244903[0] = state[3];
}
void H_27(double *state, double *unused, double *out_6659697572058893820) {
   out_6659697572058893820[0] = 0;
   out_6659697572058893820[1] = 0;
   out_6659697572058893820[2] = 0;
   out_6659697572058893820[3] = 1;
   out_6659697572058893820[4] = 0;
   out_6659697572058893820[5] = 0;
   out_6659697572058893820[6] = 0;
   out_6659697572058893820[7] = 0;
   out_6659697572058893820[8] = 0;
}
void h_29(double *state, double *unused, double *out_4435462544591115758) {
   out_4435462544591115758[0] = state[1];
}
void H_29(double *state, double *unused, double *out_8373060298928444853) {
   out_8373060298928444853[0] = 0;
   out_8373060298928444853[1] = 1;
   out_8373060298928444853[2] = 0;
   out_8373060298928444853[3] = 0;
   out_8373060298928444853[4] = 0;
   out_8373060298928444853[5] = 0;
   out_8373060298928444853[6] = 0;
   out_8373060298928444853[7] = 0;
   out_8373060298928444853[8] = 0;
}
void h_28(double *state, double *unused, double *out_1555863161497912218) {
   out_1555863161497912218[0] = state[0];
}
void H_28(double *state, double *unused, double *out_4991284757711576189) {
   out_4991284757711576189[0] = 1;
   out_4991284757711576189[1] = 0;
   out_4991284757711576189[2] = 0;
   out_4991284757711576189[3] = 0;
   out_4991284757711576189[4] = 0;
   out_4991284757711576189[5] = 0;
   out_4991284757711576189[6] = 0;
   out_4991284757711576189[7] = 0;
   out_4991284757711576189[8] = 0;
}
void h_31(double *state, double *unused, double *out_6996851981194895905) {
   out_6996851981194895905[0] = state[8];
}
void H_31(double *state, double *unused, double *out_4324949351238268411) {
   out_4324949351238268411[0] = 0;
   out_4324949351238268411[1] = 0;
   out_4324949351238268411[2] = 0;
   out_4324949351238268411[3] = 0;
   out_4324949351238268411[4] = 0;
   out_4324949351238268411[5] = 0;
   out_4324949351238268411[6] = 0;
   out_4324949351238268411[7] = 0;
   out_4324949351238268411[8] = 1;
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
void car_err_fun(double *nom_x, double *delta_x, double *out_8491087941299164185) {
  err_fun(nom_x, delta_x, out_8491087941299164185);
}
void car_inv_err_fun(double *nom_x, double *true_x, double *out_795413330377817649) {
  inv_err_fun(nom_x, true_x, out_795413330377817649);
}
void car_H_mod_fun(double *state, double *out_7465730014062278352) {
  H_mod_fun(state, out_7465730014062278352);
}
void car_f_fun(double *state, double dt, double *out_7440235772468318890) {
  f_fun(state,  dt, out_7440235772468318890);
}
void car_F_fun(double *state, double dt, double *out_2286988896813116968) {
  F_fun(state,  dt, out_2286988896813116968);
}
void car_h_25(double *state, double *unused, double *out_6851332156974240934) {
  h_25(state, unused, out_6851332156974240934);
}
void car_H_25(double *state, double *unused, double *out_4355595313115228839) {
  H_25(state, unused, out_4355595313115228839);
}
void car_h_24(double *state, double *unused, double *out_579617225417493254) {
  h_24(state, unused, out_579617225417493254);
}
void car_H_24(double *state, double *unused, double *out_4826052795158567563) {
  H_24(state, unused, out_4826052795158567563);
}
void car_h_30(double *state, double *unused, double *out_2731300491842289496) {
  h_30(state, unused, out_2731300491842289496);
}
void car_H_30(double *state, double *unused, double *out_8883291643242837037) {
  H_30(state, unused, out_8883291643242837037);
}
void car_h_26(double *state, double *unused, double *out_5768086543286622181) {
  h_26(state, unused, out_5768086543286622181);
}
void car_H_26(double *state, double *unused, double *out_8097098631989285063) {
  H_26(state, unused, out_8097098631989285063);
}
void car_h_27(double *state, double *unused, double *out_4592649212942244903) {
  h_27(state, unused, out_4592649212942244903);
}
void car_H_27(double *state, double *unused, double *out_6659697572058893820) {
  H_27(state, unused, out_6659697572058893820);
}
void car_h_29(double *state, double *unused, double *out_4435462544591115758) {
  h_29(state, unused, out_4435462544591115758);
}
void car_H_29(double *state, double *unused, double *out_8373060298928444853) {
  H_29(state, unused, out_8373060298928444853);
}
void car_h_28(double *state, double *unused, double *out_1555863161497912218) {
  h_28(state, unused, out_1555863161497912218);
}
void car_H_28(double *state, double *unused, double *out_4991284757711576189) {
  H_28(state, unused, out_4991284757711576189);
}
void car_h_31(double *state, double *unused, double *out_6996851981194895905) {
  h_31(state, unused, out_6996851981194895905);
}
void car_H_31(double *state, double *unused, double *out_4324949351238268411) {
  H_31(state, unused, out_4324949351238268411);
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
