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
void live_H(double *in_vec, double *out_7446252233834316921);
void live_err_fun(double *nom_x, double *delta_x, double *out_6842941042549192040);
void live_inv_err_fun(double *nom_x, double *true_x, double *out_74854560360654336);
void live_H_mod_fun(double *state, double *out_343386356216678134);
void live_f_fun(double *state, double dt, double *out_4177406344793620452);
void live_F_fun(double *state, double dt, double *out_3168251429203467513);
void live_h_4(double *state, double *unused, double *out_7329507194989218650);
void live_H_4(double *state, double *unused, double *out_1543584661949560601);
void live_h_9(double *state, double *unused, double *out_1548410914224601434);
void live_H_9(double *state, double *unused, double *out_5743634273314886869);
void live_h_10(double *state, double *unused, double *out_4585143685233566977);
void live_H_10(double *state, double *unused, double *out_1026407160475526063);
void live_h_12(double *state, double *unused, double *out_6019396156283845327);
void live_H_12(double *state, double *unused, double *out_6123543651732889891);
void live_h_35(double *state, double *unused, double *out_1685459619850211753);
void live_H_35(double *state, double *unused, double *out_8869106684057903600);
void live_h_32(double *state, double *unused, double *out_948385566790719096);
void live_H_32(double *state, double *unused, double *out_5491901701554724916);
void live_h_13(double *state, double *unused, double *out_8604052398133706179);
void live_H_13(double *state, double *unused, double *out_7505347846558050016);
void live_h_14(double *state, double *unused, double *out_1548410914224601434);
void live_H_14(double *state, double *unused, double *out_5743634273314886869);
void live_h_33(double *state, double *unused, double *out_1518938785812606367);
void live_H_33(double *state, double *unused, double *out_6427080385012790412);
void live_predict(double *in_x, double *in_P, double *in_Q, double dt);
}