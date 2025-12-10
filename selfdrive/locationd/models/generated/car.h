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
void car_err_fun(double *nom_x, double *delta_x, double *out_8491087941299164185);
void car_inv_err_fun(double *nom_x, double *true_x, double *out_795413330377817649);
void car_H_mod_fun(double *state, double *out_7465730014062278352);
void car_f_fun(double *state, double dt, double *out_7440235772468318890);
void car_F_fun(double *state, double dt, double *out_2286988896813116968);
void car_h_25(double *state, double *unused, double *out_6851332156974240934);
void car_H_25(double *state, double *unused, double *out_4355595313115228839);
void car_h_24(double *state, double *unused, double *out_579617225417493254);
void car_H_24(double *state, double *unused, double *out_4826052795158567563);
void car_h_30(double *state, double *unused, double *out_2731300491842289496);
void car_H_30(double *state, double *unused, double *out_8883291643242837037);
void car_h_26(double *state, double *unused, double *out_5768086543286622181);
void car_H_26(double *state, double *unused, double *out_8097098631989285063);
void car_h_27(double *state, double *unused, double *out_4592649212942244903);
void car_H_27(double *state, double *unused, double *out_6659697572058893820);
void car_h_29(double *state, double *unused, double *out_4435462544591115758);
void car_H_29(double *state, double *unused, double *out_8373060298928444853);
void car_h_28(double *state, double *unused, double *out_1555863161497912218);
void car_H_28(double *state, double *unused, double *out_4991284757711576189);
void car_h_31(double *state, double *unused, double *out_6996851981194895905);
void car_H_31(double *state, double *unused, double *out_4324949351238268411);
void car_predict(double *in_x, double *in_P, double *in_Q, double dt);
void car_set_mass(double x);
void car_set_rotational_inertia(double x);
void car_set_center_to_front(double x);
void car_set_center_to_rear(double x);
void car_set_stiffness_front(double x);
void car_set_stiffness_rear(double x);
}