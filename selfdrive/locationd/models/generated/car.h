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
void car_err_fun(double *nom_x, double *delta_x, double *out_1529549798955663691);
void car_inv_err_fun(double *nom_x, double *true_x, double *out_1921756172649404678);
void car_H_mod_fun(double *state, double *out_6794177651193603429);
void car_f_fun(double *state, double dt, double *out_8563069429564835037);
void car_F_fun(double *state, double dt, double *out_4848407684010750625);
void car_h_25(double *state, double *unused, double *out_6300616751131509609);
void car_H_25(double *state, double *unused, double *out_3840089371798884113);
void car_h_24(double *state, double *unused, double *out_4561956304062125783);
void car_H_24(double *state, double *unused, double *out_286728597598211384);
void car_h_30(double *state, double *unused, double *out_6249656637021483022);
void car_H_30(double *state, double *unused, double *out_6358422330306132740);
void car_h_26(double *state, double *unused, double *out_3645251316212785371);
void car_H_26(double *state, double *unused, double *out_98586052924827889);
void car_h_27(double *state, double *unused, double *out_7468360552207651346);
void car_H_27(double *state, double *unused, double *out_4183659018505707829);
void car_h_29(double *state, double *unused, double *out_7625547220558780491);
void car_H_29(double *state, double *unused, double *out_6868653674620524924);
void car_h_28(double *state, double *unused, double *out_6865044667531173829);
void car_H_28(double *state, double *unused, double *out_1786254657550994350);
void car_h_31(double *state, double *unused, double *out_2339260725587032275);
void car_H_31(double *state, double *unused, double *out_527622049308523587);
void car_predict(double *in_x, double *in_P, double *in_Q, double dt);
void car_set_mass(double x);
void car_set_rotational_inertia(double x);
void car_set_center_to_front(double x);
void car_set_center_to_rear(double x);
void car_set_stiffness_front(double x);
void car_set_stiffness_rear(double x);
}