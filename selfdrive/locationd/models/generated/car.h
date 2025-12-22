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
void car_err_fun(double *nom_x, double *delta_x, double *out_4017150893924662285);
void car_inv_err_fun(double *nom_x, double *true_x, double *out_1278396702776297538);
void car_H_mod_fun(double *state, double *out_4207896994963607203);
void car_f_fun(double *state, double dt, double *out_6269158699136268792);
void car_F_fun(double *state, double dt, double *out_4428556412686188077);
void car_h_25(double *state, double *unused, double *out_2015506960498938756);
void car_H_25(double *state, double *unused, double *out_3702562552763310491);
void car_h_24(double *state, double *unused, double *out_8558312059642240844);
void car_H_24(double *state, double *unused, double *out_4173020034806649215);
void car_h_30(double *state, double *unused, double *out_4139940235341721373);
void car_H_30(double *state, double *unused, double *out_3831901499906550561);
void car_h_26(double *state, double *unused, double *out_861733032319124272);
void car_H_26(double *state, double *unused, double *out_7444065871637366715);
void car_h_27(double *state, double *unused, double *out_9220327626956391349);
void car_H_27(double *state, double *unused, double *out_6006664811706975472);
void car_h_29(double *state, double *unused, double *out_2449492400606040413);
void car_H_29(double *state, double *unused, double *out_3321670155592158377);
void car_h_28(double *state, double *unused, double *out_8052583825880249612);
void car_H_28(double *state, double *unused, double *out_1358039884026832126);
void car_h_31(double *state, double *unused, double *out_4755328265851412180);
void car_H_31(double *state, double *unused, double *out_3671916590886350063);
void car_predict(double *in_x, double *in_P, double *in_Q, double dt);
void car_set_mass(double x);
void car_set_rotational_inertia(double x);
void car_set_center_to_front(double x);
void car_set_center_to_rear(double x);
void car_set_stiffness_front(double x);
void car_set_stiffness_rear(double x);
}