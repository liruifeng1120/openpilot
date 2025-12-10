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
void live_H(double *in_vec, double *out_7587996004209891357);
void live_err_fun(double *nom_x, double *delta_x, double *out_3292393683986331180);
void live_inv_err_fun(double *nom_x, double *true_x, double *out_422608199939508949);
void live_H_mod_fun(double *state, double *out_4663896803907986165);
void live_f_fun(double *state, double dt, double *out_1670747687864062567);
void live_F_fun(double *state, double dt, double *out_7114411898040405356);
void live_h_4(double *state, double *unused, double *out_692698749186459348);
void live_H_4(double *state, double *unused, double *out_7496522684116631469);
void live_h_9(double *state, double *unused, double *out_8534772652916089139);
void live_H_9(double *state, double *unused, double *out_7737712330746222114);
void live_h_10(double *state, double *unused, double *out_3893642475585480214);
void live_H_10(double *state, double *unused, double *out_463725680919038262);
void live_h_12(double *state, double *unused, double *out_3325230464569597786);
void live_H_12(double *state, double *unused, double *out_5930764981560958352);
void live_h_35(double *state, double *unused, double *out_8307693605957998854);
void live_H_35(double *state, double *unused, double *out_3185201949235944643);
void live_h_32(double *state, double *unused, double *out_7346946874137284545);
void live_H_32(double *state, double *unused, double *out_362642390751214086);
void live_h_13(double *state, double *unused, double *out_8866436180359755168);
void live_H_13(double *state, double *unused, double *out_6709834338183679718);
void live_h_14(double *state, double *unused, double *out_8534772652916089139);
void live_H_14(double *state, double *unused, double *out_7737712330746222114);
void live_h_33(double *state, double *unused, double *out_2806934353530501133);
void live_H_33(double *state, double *unused, double *out_34644944597087039);
void live_predict(double *in_x, double *in_P, double *in_Q, double dt);
}