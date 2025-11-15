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
void live_H(double *in_vec, double *out_5642657983657145134);
void live_err_fun(double *nom_x, double *delta_x, double *out_9193783301721394327);
void live_inv_err_fun(double *nom_x, double *true_x, double *out_4650060410524811721);
void live_H_mod_fun(double *state, double *out_5304395359108658320);
void live_f_fun(double *state, double dt, double *out_1606848742264401268);
void live_F_fun(double *state, double dt, double *out_3187982528965108114);
void live_h_4(double *state, double *unused, double *out_3910570347799487406);
void live_H_4(double *state, double *unused, double *out_4704253365926979905);
void live_h_9(double *state, double *unused, double *out_7388957791785279159);
void live_H_9(double *state, double *unused, double *out_6455271772518124241);
void live_h_10(double *state, double *unused, double *out_5805874686487777876);
void live_H_10(double *state, double *unused, double *out_1507386169655748480);
void live_h_12(double *state, double *unused, double *out_3656602660648231056);
void live_H_12(double *state, double *unused, double *out_8723034299750609916);
void live_h_35(double *state, double *unused, double *out_1047246714872217456);
void live_H_35(double *state, double *unused, double *out_5977471267425596207);
void live_h_32(double *state, double *unused, double *out_8745115064712854771);
void live_H_32(double *state, double *unused, double *out_3852216185627678211);
void live_h_13(double *state, double *unused, double *out_241279307899422473);
void live_H_13(double *state, double *unused, double *out_727729041821352744);
void live_h_14(double *state, double *unused, double *out_7388957791785279159);
void live_H_14(double *state, double *unused, double *out_6455271772518124241);
void live_h_33(double *state, double *unused, double *out_6900089178888289432);
void live_H_33(double *state, double *unused, double *out_2826914262786738603);
void live_predict(double *in_x, double *in_P, double *in_Q, double dt);
}