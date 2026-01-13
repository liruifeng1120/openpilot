#pragma once
#include "rednose/helpers/ekf.h"
extern "C" {
void car_update_25(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void car_update_24(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void car_update_30(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void car_update_26(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void car_update_27(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void car_update_29(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void car_update_28(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void car_update_31(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void car_err_fun(double *nom_x, double *delta_x, double *out_4516286648636903722);
void car_inv_err_fun(double *nom_x, double *true_x, double *out_28703963116213310);
void car_H_mod_fun(double *state, double *out_8940306794122284730);
void car_f_fun(double *state, double dt, double *out_5837366013746318941);
void car_F_fun(double *state, double dt, double *out_5019779894623636516);
void car_h_25(double *state, double *unused, double *out_3339146431292074036);
void car_H_25(double *state, double *unused, double *out_5417326144132793053);
void car_h_24(double *state, double *unused, double *out_2648481969623238997);
void car_H_24(double *state, double *unused, double *out_3191618360153924491);
void car_h_30(double *state, double *unused, double *out_8336762416067126644);
void car_H_30(double *state, double *unused, double *out_1499364197358823702);
void car_h_26(double *state, double *unused, double *out_1544519442435394865);
void car_H_26(double *state, double *unused, double *out_9158829463006849277);
void car_h_27(double *state, double *unused, double *out_5457163032973923104);
void car_H_27(double *state, double *unused, double *out_7721428403076458034);
void car_h_29(double *state, double *unused, double *out_6834133844894968383);
void car_H_29(double *state, double *unused, double *out_5036433746961640939);
void car_h_28(double *state, double *unused, double *out_6284712865824879923);
void car_H_28(double *state, double *unused, double *out_3072803475396314688);
void car_h_31(double *state, double *unused, double *out_3181959762940944891);
void car_H_31(double *state, double *unused, double *out_5386680182255832625);
void car_predict(double *in_x, double *in_P, double *in_Q, double dt);
void car_set_mass(double x);
void car_set_rotational_inertia(double x);
void car_set_center_to_front(double x);
void car_set_center_to_rear(double x);
void car_set_stiffness_front(double x);
void car_set_stiffness_rear(double x);
}