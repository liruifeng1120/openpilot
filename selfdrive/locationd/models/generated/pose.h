#pragma once
#include "rednose/helpers/ekf.h"
extern "C" {
void pose_update_4(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void pose_update_10(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void pose_update_13(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void pose_update_14(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void pose_err_fun(double *nom_x, double *delta_x, double *out_528651136918936599);
void pose_inv_err_fun(double *nom_x, double *true_x, double *out_6367489553962069677);
void pose_H_mod_fun(double *state, double *out_449719862301799713);
void pose_f_fun(double *state, double dt, double *out_4643815075913585868);
void pose_F_fun(double *state, double dt, double *out_7345596707252897319);
void pose_h_4(double *state, double *unused, double *out_2234390001006855485);
void pose_H_4(double *state, double *unused, double *out_3723941941092748966);
void pose_h_10(double *state, double *unused, double *out_7730713563924800251);
void pose_H_10(double *state, double *unused, double *out_8748160934080505697);
void pose_h_13(double *state, double *unused, double *out_3777997688627648949);
void pose_H_13(double *state, double *unused, double *out_3886689267223951963);
void pose_h_14(double *state, double *unused, double *out_131601925830122397);
void pose_H_14(double *state, double *unused, double *out_239298915246735563);
void pose_predict(double *in_x, double *in_P, double *in_Q, double dt);
}