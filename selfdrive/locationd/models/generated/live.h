#pragma once
#include "rednose/helpers/ekf.h"
extern "C" {
void live_update_4(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void live_update_9(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void live_update_10(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void live_update_12(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void live_update_35(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void live_update_32(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void live_update_13(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void live_update_14(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void live_update_33(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void live_H(double *in_vec, double *out_7790294567976970969);
void live_err_fun(double *nom_x, double *delta_x, double *out_5556001192030736320);
void live_inv_err_fun(double *nom_x, double *true_x, double *out_5573260547778088089);
void live_H_mod_fun(double *state, double *out_8583959898598634538);
void live_f_fun(double *state, double dt, double *out_1720292329662776750);
void live_F_fun(double *state, double dt, double *out_1444964209229130545);
void live_h_4(double *state, double *unused, double *out_1216635394173073273);
void live_H_4(double *state, double *unused, double *out_2207723470307906526);
void live_h_9(double *state, double *unused, double *out_7288997930029212347);
void live_H_9(double *state, double *unused, double *out_8951801668137197620);
void live_h_10(double *state, double *unused, double *out_637054798376112);
void live_H_10(double *state, double *unused, double *out_3264415378508357904);
void live_h_12(double *state, double *unused, double *out_144896252467199760);
void live_H_12(double *state, double *unused, double *out_7227179878339868321);
void live_h_35(double *state, double *unused, double *out_3373201461079988444);
void live_H_35(double *state, double *unused, double *out_5574385527680513902);
void live_h_32(double *state, double *unused, double *out_5746804499450278138);
void live_H_32(double *state, double *unused, double *out_3929546017969614300);
void live_h_13(double *state, double *unused, double *out_4686749418951650756);
void live_H_13(double *state, double *unused, double *out_7729380554194393999);
void live_h_14(double *state, double *unused, double *out_7288997930029212347);
void live_H_14(double *state, double *unused, double *out_8951801668137197620);
void live_h_33(double *state, double *unused, double *out_8077932856284141760);
void live_H_33(double *state, double *unused, double *out_8724942532319371506);
void live_predict(double *in_x, double *in_P, double *in_Q, double dt);
}