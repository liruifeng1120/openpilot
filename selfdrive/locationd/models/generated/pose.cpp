#include "pose.h"

namespace {
#define DIM 18
#define EDIM 18
#define MEDIM 18
typedef void (*Hfun)(double *, double *, double *);
const static double MAHA_THRESH_4 = 7.814727903251177;
const static double MAHA_THRESH_10 = 7.814727903251177;
const static double MAHA_THRESH_13 = 7.814727903251177;
const static double MAHA_THRESH_14 = 7.814727903251177;

/******************************************************************************
 *                      Code generated with SymPy 1.14.0                      *
 *                                                                            *
 *              See http://www.sympy.org/ for more information.               *
 *                                                                            *
 *                         This file is part of 'ekf'                         *
 ******************************************************************************/
void err_fun(double *nom_x, double *delta_x, double *out_528651136918936599) {
   out_528651136918936599[0] = delta_x[0] + nom_x[0];
   out_528651136918936599[1] = delta_x[1] + nom_x[1];
   out_528651136918936599[2] = delta_x[2] + nom_x[2];
   out_528651136918936599[3] = delta_x[3] + nom_x[3];
   out_528651136918936599[4] = delta_x[4] + nom_x[4];
   out_528651136918936599[5] = delta_x[5] + nom_x[5];
   out_528651136918936599[6] = delta_x[6] + nom_x[6];
   out_528651136918936599[7] = delta_x[7] + nom_x[7];
   out_528651136918936599[8] = delta_x[8] + nom_x[8];
   out_528651136918936599[9] = delta_x[9] + nom_x[9];
   out_528651136918936599[10] = delta_x[10] + nom_x[10];
   out_528651136918936599[11] = delta_x[11] + nom_x[11];
   out_528651136918936599[12] = delta_x[12] + nom_x[12];
   out_528651136918936599[13] = delta_x[13] + nom_x[13];
   out_528651136918936599[14] = delta_x[14] + nom_x[14];
   out_528651136918936599[15] = delta_x[15] + nom_x[15];
   out_528651136918936599[16] = delta_x[16] + nom_x[16];
   out_528651136918936599[17] = delta_x[17] + nom_x[17];
}
void inv_err_fun(double *nom_x, double *true_x, double *out_6367489553962069677) {
   out_6367489553962069677[0] = -nom_x[0] + true_x[0];
   out_6367489553962069677[1] = -nom_x[1] + true_x[1];
   out_6367489553962069677[2] = -nom_x[2] + true_x[2];
   out_6367489553962069677[3] = -nom_x[3] + true_x[3];
   out_6367489553962069677[4] = -nom_x[4] + true_x[4];
   out_6367489553962069677[5] = -nom_x[5] + true_x[5];
   out_6367489553962069677[6] = -nom_x[6] + true_x[6];
   out_6367489553962069677[7] = -nom_x[7] + true_x[7];
   out_6367489553962069677[8] = -nom_x[8] + true_x[8];
   out_6367489553962069677[9] = -nom_x[9] + true_x[9];
   out_6367489553962069677[10] = -nom_x[10] + true_x[10];
   out_6367489553962069677[11] = -nom_x[11] + true_x[11];
   out_6367489553962069677[12] = -nom_x[12] + true_x[12];
   out_6367489553962069677[13] = -nom_x[13] + true_x[13];
   out_6367489553962069677[14] = -nom_x[14] + true_x[14];
   out_6367489553962069677[15] = -nom_x[15] + true_x[15];
   out_6367489553962069677[16] = -nom_x[16] + true_x[16];
   out_6367489553962069677[17] = -nom_x[17] + true_x[17];
}
void H_mod_fun(double *state, double *out_449719862301799713) {
   out_449719862301799713[0] = 1.0;
   out_449719862301799713[1] = 0.0;
   out_449719862301799713[2] = 0.0;
   out_449719862301799713[3] = 0.0;
   out_449719862301799713[4] = 0.0;
   out_449719862301799713[5] = 0.0;
   out_449719862301799713[6] = 0.0;
   out_449719862301799713[7] = 0.0;
   out_449719862301799713[8] = 0.0;
   out_449719862301799713[9] = 0.0;
   out_449719862301799713[10] = 0.0;
   out_449719862301799713[11] = 0.0;
   out_449719862301799713[12] = 0.0;
   out_449719862301799713[13] = 0.0;
   out_449719862301799713[14] = 0.0;
   out_449719862301799713[15] = 0.0;
   out_449719862301799713[16] = 0.0;
   out_449719862301799713[17] = 0.0;
   out_449719862301799713[18] = 0.0;
   out_449719862301799713[19] = 1.0;
   out_449719862301799713[20] = 0.0;
   out_449719862301799713[21] = 0.0;
   out_449719862301799713[22] = 0.0;
   out_449719862301799713[23] = 0.0;
   out_449719862301799713[24] = 0.0;
   out_449719862301799713[25] = 0.0;
   out_449719862301799713[26] = 0.0;
   out_449719862301799713[27] = 0.0;
   out_449719862301799713[28] = 0.0;
   out_449719862301799713[29] = 0.0;
   out_449719862301799713[30] = 0.0;
   out_449719862301799713[31] = 0.0;
   out_449719862301799713[32] = 0.0;
   out_449719862301799713[33] = 0.0;
   out_449719862301799713[34] = 0.0;
   out_449719862301799713[35] = 0.0;
   out_449719862301799713[36] = 0.0;
   out_449719862301799713[37] = 0.0;
   out_449719862301799713[38] = 1.0;
   out_449719862301799713[39] = 0.0;
   out_449719862301799713[40] = 0.0;
   out_449719862301799713[41] = 0.0;
   out_449719862301799713[42] = 0.0;
   out_449719862301799713[43] = 0.0;
   out_449719862301799713[44] = 0.0;
   out_449719862301799713[45] = 0.0;
   out_449719862301799713[46] = 0.0;
   out_449719862301799713[47] = 0.0;
   out_449719862301799713[48] = 0.0;
   out_449719862301799713[49] = 0.0;
   out_449719862301799713[50] = 0.0;
   out_449719862301799713[51] = 0.0;
   out_449719862301799713[52] = 0.0;
   out_449719862301799713[53] = 0.0;
   out_449719862301799713[54] = 0.0;
   out_449719862301799713[55] = 0.0;
   out_449719862301799713[56] = 0.0;
   out_449719862301799713[57] = 1.0;
   out_449719862301799713[58] = 0.0;
   out_449719862301799713[59] = 0.0;
   out_449719862301799713[60] = 0.0;
   out_449719862301799713[61] = 0.0;
   out_449719862301799713[62] = 0.0;
   out_449719862301799713[63] = 0.0;
   out_449719862301799713[64] = 0.0;
   out_449719862301799713[65] = 0.0;
   out_449719862301799713[66] = 0.0;
   out_449719862301799713[67] = 0.0;
   out_449719862301799713[68] = 0.0;
   out_449719862301799713[69] = 0.0;
   out_449719862301799713[70] = 0.0;
   out_449719862301799713[71] = 0.0;
   out_449719862301799713[72] = 0.0;
   out_449719862301799713[73] = 0.0;
   out_449719862301799713[74] = 0.0;
   out_449719862301799713[75] = 0.0;
   out_449719862301799713[76] = 1.0;
   out_449719862301799713[77] = 0.0;
   out_449719862301799713[78] = 0.0;
   out_449719862301799713[79] = 0.0;
   out_449719862301799713[80] = 0.0;
   out_449719862301799713[81] = 0.0;
   out_449719862301799713[82] = 0.0;
   out_449719862301799713[83] = 0.0;
   out_449719862301799713[84] = 0.0;
   out_449719862301799713[85] = 0.0;
   out_449719862301799713[86] = 0.0;
   out_449719862301799713[87] = 0.0;
   out_449719862301799713[88] = 0.0;
   out_449719862301799713[89] = 0.0;
   out_449719862301799713[90] = 0.0;
   out_449719862301799713[91] = 0.0;
   out_449719862301799713[92] = 0.0;
   out_449719862301799713[93] = 0.0;
   out_449719862301799713[94] = 0.0;
   out_449719862301799713[95] = 1.0;
   out_449719862301799713[96] = 0.0;
   out_449719862301799713[97] = 0.0;
   out_449719862301799713[98] = 0.0;
   out_449719862301799713[99] = 0.0;
   out_449719862301799713[100] = 0.0;
   out_449719862301799713[101] = 0.0;
   out_449719862301799713[102] = 0.0;
   out_449719862301799713[103] = 0.0;
   out_449719862301799713[104] = 0.0;
   out_449719862301799713[105] = 0.0;
   out_449719862301799713[106] = 0.0;
   out_449719862301799713[107] = 0.0;
   out_449719862301799713[108] = 0.0;
   out_449719862301799713[109] = 0.0;
   out_449719862301799713[110] = 0.0;
   out_449719862301799713[111] = 0.0;
   out_449719862301799713[112] = 0.0;
   out_449719862301799713[113] = 0.0;
   out_449719862301799713[114] = 1.0;
   out_449719862301799713[115] = 0.0;
   out_449719862301799713[116] = 0.0;
   out_449719862301799713[117] = 0.0;
   out_449719862301799713[118] = 0.0;
   out_449719862301799713[119] = 0.0;
   out_449719862301799713[120] = 0.0;
   out_449719862301799713[121] = 0.0;
   out_449719862301799713[122] = 0.0;
   out_449719862301799713[123] = 0.0;
   out_449719862301799713[124] = 0.0;
   out_449719862301799713[125] = 0.0;
   out_449719862301799713[126] = 0.0;
   out_449719862301799713[127] = 0.0;
   out_449719862301799713[128] = 0.0;
   out_449719862301799713[129] = 0.0;
   out_449719862301799713[130] = 0.0;
   out_449719862301799713[131] = 0.0;
   out_449719862301799713[132] = 0.0;
   out_449719862301799713[133] = 1.0;
   out_449719862301799713[134] = 0.0;
   out_449719862301799713[135] = 0.0;
   out_449719862301799713[136] = 0.0;
   out_449719862301799713[137] = 0.0;
   out_449719862301799713[138] = 0.0;
   out_449719862301799713[139] = 0.0;
   out_449719862301799713[140] = 0.0;
   out_449719862301799713[141] = 0.0;
   out_449719862301799713[142] = 0.0;
   out_449719862301799713[143] = 0.0;
   out_449719862301799713[144] = 0.0;
   out_449719862301799713[145] = 0.0;
   out_449719862301799713[146] = 0.0;
   out_449719862301799713[147] = 0.0;
   out_449719862301799713[148] = 0.0;
   out_449719862301799713[149] = 0.0;
   out_449719862301799713[150] = 0.0;
   out_449719862301799713[151] = 0.0;
   out_449719862301799713[152] = 1.0;
   out_449719862301799713[153] = 0.0;
   out_449719862301799713[154] = 0.0;
   out_449719862301799713[155] = 0.0;
   out_449719862301799713[156] = 0.0;
   out_449719862301799713[157] = 0.0;
   out_449719862301799713[158] = 0.0;
   out_449719862301799713[159] = 0.0;
   out_449719862301799713[160] = 0.0;
   out_449719862301799713[161] = 0.0;
   out_449719862301799713[162] = 0.0;
   out_449719862301799713[163] = 0.0;
   out_449719862301799713[164] = 0.0;
   out_449719862301799713[165] = 0.0;
   out_449719862301799713[166] = 0.0;
   out_449719862301799713[167] = 0.0;
   out_449719862301799713[168] = 0.0;
   out_449719862301799713[169] = 0.0;
   out_449719862301799713[170] = 0.0;
   out_449719862301799713[171] = 1.0;
   out_449719862301799713[172] = 0.0;
   out_449719862301799713[173] = 0.0;
   out_449719862301799713[174] = 0.0;
   out_449719862301799713[175] = 0.0;
   out_449719862301799713[176] = 0.0;
   out_449719862301799713[177] = 0.0;
   out_449719862301799713[178] = 0.0;
   out_449719862301799713[179] = 0.0;
   out_449719862301799713[180] = 0.0;
   out_449719862301799713[181] = 0.0;
   out_449719862301799713[182] = 0.0;
   out_449719862301799713[183] = 0.0;
   out_449719862301799713[184] = 0.0;
   out_449719862301799713[185] = 0.0;
   out_449719862301799713[186] = 0.0;
   out_449719862301799713[187] = 0.0;
   out_449719862301799713[188] = 0.0;
   out_449719862301799713[189] = 0.0;
   out_449719862301799713[190] = 1.0;
   out_449719862301799713[191] = 0.0;
   out_449719862301799713[192] = 0.0;
   out_449719862301799713[193] = 0.0;
   out_449719862301799713[194] = 0.0;
   out_449719862301799713[195] = 0.0;
   out_449719862301799713[196] = 0.0;
   out_449719862301799713[197] = 0.0;
   out_449719862301799713[198] = 0.0;
   out_449719862301799713[199] = 0.0;
   out_449719862301799713[200] = 0.0;
   out_449719862301799713[201] = 0.0;
   out_449719862301799713[202] = 0.0;
   out_449719862301799713[203] = 0.0;
   out_449719862301799713[204] = 0.0;
   out_449719862301799713[205] = 0.0;
   out_449719862301799713[206] = 0.0;
   out_449719862301799713[207] = 0.0;
   out_449719862301799713[208] = 0.0;
   out_449719862301799713[209] = 1.0;
   out_449719862301799713[210] = 0.0;
   out_449719862301799713[211] = 0.0;
   out_449719862301799713[212] = 0.0;
   out_449719862301799713[213] = 0.0;
   out_449719862301799713[214] = 0.0;
   out_449719862301799713[215] = 0.0;
   out_449719862301799713[216] = 0.0;
   out_449719862301799713[217] = 0.0;
   out_449719862301799713[218] = 0.0;
   out_449719862301799713[219] = 0.0;
   out_449719862301799713[220] = 0.0;
   out_449719862301799713[221] = 0.0;
   out_449719862301799713[222] = 0.0;
   out_449719862301799713[223] = 0.0;
   out_449719862301799713[224] = 0.0;
   out_449719862301799713[225] = 0.0;
   out_449719862301799713[226] = 0.0;
   out_449719862301799713[227] = 0.0;
   out_449719862301799713[228] = 1.0;
   out_449719862301799713[229] = 0.0;
   out_449719862301799713[230] = 0.0;
   out_449719862301799713[231] = 0.0;
   out_449719862301799713[232] = 0.0;
   out_449719862301799713[233] = 0.0;
   out_449719862301799713[234] = 0.0;
   out_449719862301799713[235] = 0.0;
   out_449719862301799713[236] = 0.0;
   out_449719862301799713[237] = 0.0;
   out_449719862301799713[238] = 0.0;
   out_449719862301799713[239] = 0.0;
   out_449719862301799713[240] = 0.0;
   out_449719862301799713[241] = 0.0;
   out_449719862301799713[242] = 0.0;
   out_449719862301799713[243] = 0.0;
   out_449719862301799713[244] = 0.0;
   out_449719862301799713[245] = 0.0;
   out_449719862301799713[246] = 0.0;
   out_449719862301799713[247] = 1.0;
   out_449719862301799713[248] = 0.0;
   out_449719862301799713[249] = 0.0;
   out_449719862301799713[250] = 0.0;
   out_449719862301799713[251] = 0.0;
   out_449719862301799713[252] = 0.0;
   out_449719862301799713[253] = 0.0;
   out_449719862301799713[254] = 0.0;
   out_449719862301799713[255] = 0.0;
   out_449719862301799713[256] = 0.0;
   out_449719862301799713[257] = 0.0;
   out_449719862301799713[258] = 0.0;
   out_449719862301799713[259] = 0.0;
   out_449719862301799713[260] = 0.0;
   out_449719862301799713[261] = 0.0;
   out_449719862301799713[262] = 0.0;
   out_449719862301799713[263] = 0.0;
   out_449719862301799713[264] = 0.0;
   out_449719862301799713[265] = 0.0;
   out_449719862301799713[266] = 1.0;
   out_449719862301799713[267] = 0.0;
   out_449719862301799713[268] = 0.0;
   out_449719862301799713[269] = 0.0;
   out_449719862301799713[270] = 0.0;
   out_449719862301799713[271] = 0.0;
   out_449719862301799713[272] = 0.0;
   out_449719862301799713[273] = 0.0;
   out_449719862301799713[274] = 0.0;
   out_449719862301799713[275] = 0.0;
   out_449719862301799713[276] = 0.0;
   out_449719862301799713[277] = 0.0;
   out_449719862301799713[278] = 0.0;
   out_449719862301799713[279] = 0.0;
   out_449719862301799713[280] = 0.0;
   out_449719862301799713[281] = 0.0;
   out_449719862301799713[282] = 0.0;
   out_449719862301799713[283] = 0.0;
   out_449719862301799713[284] = 0.0;
   out_449719862301799713[285] = 1.0;
   out_449719862301799713[286] = 0.0;
   out_449719862301799713[287] = 0.0;
   out_449719862301799713[288] = 0.0;
   out_449719862301799713[289] = 0.0;
   out_449719862301799713[290] = 0.0;
   out_449719862301799713[291] = 0.0;
   out_449719862301799713[292] = 0.0;
   out_449719862301799713[293] = 0.0;
   out_449719862301799713[294] = 0.0;
   out_449719862301799713[295] = 0.0;
   out_449719862301799713[296] = 0.0;
   out_449719862301799713[297] = 0.0;
   out_449719862301799713[298] = 0.0;
   out_449719862301799713[299] = 0.0;
   out_449719862301799713[300] = 0.0;
   out_449719862301799713[301] = 0.0;
   out_449719862301799713[302] = 0.0;
   out_449719862301799713[303] = 0.0;
   out_449719862301799713[304] = 1.0;
   out_449719862301799713[305] = 0.0;
   out_449719862301799713[306] = 0.0;
   out_449719862301799713[307] = 0.0;
   out_449719862301799713[308] = 0.0;
   out_449719862301799713[309] = 0.0;
   out_449719862301799713[310] = 0.0;
   out_449719862301799713[311] = 0.0;
   out_449719862301799713[312] = 0.0;
   out_449719862301799713[313] = 0.0;
   out_449719862301799713[314] = 0.0;
   out_449719862301799713[315] = 0.0;
   out_449719862301799713[316] = 0.0;
   out_449719862301799713[317] = 0.0;
   out_449719862301799713[318] = 0.0;
   out_449719862301799713[319] = 0.0;
   out_449719862301799713[320] = 0.0;
   out_449719862301799713[321] = 0.0;
   out_449719862301799713[322] = 0.0;
   out_449719862301799713[323] = 1.0;
}
void f_fun(double *state, double dt, double *out_4643815075913585868) {
   out_4643815075913585868[0] = atan2((sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) - (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) + sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), -(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]));
   out_4643815075913585868[1] = asin(sin(dt*state[7])*cos(state[0])*cos(state[1]) - sin(dt*state[8])*sin(state[0])*cos(dt*state[7])*cos(state[1]) + sin(state[1])*cos(dt*state[7])*cos(dt*state[8]));
   out_4643815075913585868[2] = atan2(-(-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) + sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]), -(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]));
   out_4643815075913585868[3] = dt*state[12] + state[3];
   out_4643815075913585868[4] = dt*state[13] + state[4];
   out_4643815075913585868[5] = dt*state[14] + state[5];
   out_4643815075913585868[6] = state[6];
   out_4643815075913585868[7] = state[7];
   out_4643815075913585868[8] = state[8];
   out_4643815075913585868[9] = state[9];
   out_4643815075913585868[10] = state[10];
   out_4643815075913585868[11] = state[11];
   out_4643815075913585868[12] = state[12];
   out_4643815075913585868[13] = state[13];
   out_4643815075913585868[14] = state[14];
   out_4643815075913585868[15] = state[15];
   out_4643815075913585868[16] = state[16];
   out_4643815075913585868[17] = state[17];
}
void F_fun(double *state, double dt, double *out_7345596707252897319) {
   out_7345596707252897319[0] = ((-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*cos(state[0])*cos(state[1]) - sin(state[0])*cos(dt*state[6])*cos(dt*state[7])*cos(state[1]))*(-(sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) + (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) - sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]))/(pow(-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2) + pow((sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) - (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) + sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2)) + ((sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*cos(state[0])*cos(state[1]) - sin(dt*state[6])*sin(state[0])*cos(dt*state[7])*cos(state[1]))*(-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]))/(pow(-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2) + pow((sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) - (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) + sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2));
   out_7345596707252897319[1] = ((-sin(dt*state[6])*sin(dt*state[8]) - sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*cos(state[1]) - (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*sin(state[1]) - sin(state[1])*cos(dt*state[6])*cos(dt*state[7])*cos(state[0]))*(-(sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) + (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) - sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]))/(pow(-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2) + pow((sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) - (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) + sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2)) + (-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]))*(-(sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*sin(state[1]) + (-sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) + sin(dt*state[8])*cos(dt*state[6]))*cos(state[1]) - sin(dt*state[6])*sin(state[1])*cos(dt*state[7])*cos(state[0]))/(pow(-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2) + pow((sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) - (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) + sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2));
   out_7345596707252897319[2] = 0;
   out_7345596707252897319[3] = 0;
   out_7345596707252897319[4] = 0;
   out_7345596707252897319[5] = 0;
   out_7345596707252897319[6] = (-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]))*(dt*cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]) + (-dt*sin(dt*state[6])*sin(dt*state[8]) - dt*sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-dt*sin(dt*state[6])*cos(dt*state[8]) + dt*sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]))/(pow(-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2) + pow((sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) - (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) + sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2)) + (-(sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) + (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) - sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]))*(-dt*sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]) + (-dt*sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) - dt*cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) + (dt*sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - dt*sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]))/(pow(-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2) + pow((sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) - (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) + sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2));
   out_7345596707252897319[7] = (-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]))*(-dt*sin(dt*state[6])*sin(dt*state[7])*cos(state[0])*cos(state[1]) + dt*sin(dt*state[6])*sin(dt*state[8])*sin(state[0])*cos(dt*state[7])*cos(state[1]) - dt*sin(dt*state[6])*sin(state[1])*cos(dt*state[7])*cos(dt*state[8]))/(pow(-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2) + pow((sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) - (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) + sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2)) + (-(sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) + (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) - sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]))*(-dt*sin(dt*state[7])*cos(dt*state[6])*cos(state[0])*cos(state[1]) + dt*sin(dt*state[8])*sin(state[0])*cos(dt*state[6])*cos(dt*state[7])*cos(state[1]) - dt*sin(state[1])*cos(dt*state[6])*cos(dt*state[7])*cos(dt*state[8]))/(pow(-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2) + pow((sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) - (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) + sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2));
   out_7345596707252897319[8] = ((dt*sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + dt*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (dt*sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - dt*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]))*(-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]))/(pow(-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2) + pow((sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) - (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) + sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2)) + ((dt*sin(dt*state[6])*sin(dt*state[8]) + dt*sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) + (-dt*sin(dt*state[6])*cos(dt*state[8]) + dt*sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]))*(-(sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) + (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) - sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]))/(pow(-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2) + pow((sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) - (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) + sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2));
   out_7345596707252897319[9] = 0;
   out_7345596707252897319[10] = 0;
   out_7345596707252897319[11] = 0;
   out_7345596707252897319[12] = 0;
   out_7345596707252897319[13] = 0;
   out_7345596707252897319[14] = 0;
   out_7345596707252897319[15] = 0;
   out_7345596707252897319[16] = 0;
   out_7345596707252897319[17] = 0;
   out_7345596707252897319[18] = (-sin(dt*state[7])*sin(state[0])*cos(state[1]) - sin(dt*state[8])*cos(dt*state[7])*cos(state[0])*cos(state[1]))/sqrt(1 - pow(sin(dt*state[7])*cos(state[0])*cos(state[1]) - sin(dt*state[8])*sin(state[0])*cos(dt*state[7])*cos(state[1]) + sin(state[1])*cos(dt*state[7])*cos(dt*state[8]), 2));
   out_7345596707252897319[19] = (-sin(dt*state[7])*sin(state[1])*cos(state[0]) + sin(dt*state[8])*sin(state[0])*sin(state[1])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1]))/sqrt(1 - pow(sin(dt*state[7])*cos(state[0])*cos(state[1]) - sin(dt*state[8])*sin(state[0])*cos(dt*state[7])*cos(state[1]) + sin(state[1])*cos(dt*state[7])*cos(dt*state[8]), 2));
   out_7345596707252897319[20] = 0;
   out_7345596707252897319[21] = 0;
   out_7345596707252897319[22] = 0;
   out_7345596707252897319[23] = 0;
   out_7345596707252897319[24] = 0;
   out_7345596707252897319[25] = (dt*sin(dt*state[7])*sin(dt*state[8])*sin(state[0])*cos(state[1]) - dt*sin(dt*state[7])*sin(state[1])*cos(dt*state[8]) + dt*cos(dt*state[7])*cos(state[0])*cos(state[1]))/sqrt(1 - pow(sin(dt*state[7])*cos(state[0])*cos(state[1]) - sin(dt*state[8])*sin(state[0])*cos(dt*state[7])*cos(state[1]) + sin(state[1])*cos(dt*state[7])*cos(dt*state[8]), 2));
   out_7345596707252897319[26] = (-dt*sin(dt*state[8])*sin(state[1])*cos(dt*state[7]) - dt*sin(state[0])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]))/sqrt(1 - pow(sin(dt*state[7])*cos(state[0])*cos(state[1]) - sin(dt*state[8])*sin(state[0])*cos(dt*state[7])*cos(state[1]) + sin(state[1])*cos(dt*state[7])*cos(dt*state[8]), 2));
   out_7345596707252897319[27] = 0;
   out_7345596707252897319[28] = 0;
   out_7345596707252897319[29] = 0;
   out_7345596707252897319[30] = 0;
   out_7345596707252897319[31] = 0;
   out_7345596707252897319[32] = 0;
   out_7345596707252897319[33] = 0;
   out_7345596707252897319[34] = 0;
   out_7345596707252897319[35] = 0;
   out_7345596707252897319[36] = ((sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[7]))*((-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) - (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) - sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]))/(pow(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]), 2) + pow(-(-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) + sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]), 2)) + ((-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[7]))*(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]))/(pow(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]), 2) + pow(-(-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) + sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]), 2));
   out_7345596707252897319[37] = (-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]))*(-sin(dt*state[7])*sin(state[2])*cos(state[0])*cos(state[1]) + sin(dt*state[8])*sin(state[0])*sin(state[2])*cos(dt*state[7])*cos(state[1]) - sin(state[1])*sin(state[2])*cos(dt*state[7])*cos(dt*state[8]))/(pow(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]), 2) + pow(-(-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) + sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]), 2)) + ((-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) - (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) - sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]))*(-sin(dt*state[7])*cos(state[0])*cos(state[1])*cos(state[2]) + sin(dt*state[8])*sin(state[0])*cos(dt*state[7])*cos(state[1])*cos(state[2]) - sin(state[1])*cos(dt*state[7])*cos(dt*state[8])*cos(state[2]))/(pow(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]), 2) + pow(-(-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) + sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]), 2));
   out_7345596707252897319[38] = ((-sin(state[0])*sin(state[2]) - sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]))*(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]))/(pow(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]), 2) + pow(-(-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) + sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]), 2)) + ((-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) + (-sin(state[0])*sin(state[1])*sin(state[2]) - cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) - sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]))*((-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) - (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) - sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]))/(pow(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]), 2) + pow(-(-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) + sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]), 2));
   out_7345596707252897319[39] = 0;
   out_7345596707252897319[40] = 0;
   out_7345596707252897319[41] = 0;
   out_7345596707252897319[42] = 0;
   out_7345596707252897319[43] = (-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]))*(dt*(sin(state[0])*cos(state[2]) - sin(state[1])*sin(state[2])*cos(state[0]))*cos(dt*state[7]) - dt*(sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[7])*sin(dt*state[8]) - dt*sin(dt*state[7])*sin(state[2])*cos(dt*state[8])*cos(state[1]))/(pow(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]), 2) + pow(-(-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) + sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]), 2)) + ((-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) - (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) - sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]))*(dt*(-sin(state[0])*sin(state[2]) - sin(state[1])*cos(state[0])*cos(state[2]))*cos(dt*state[7]) - dt*(sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[7])*sin(dt*state[8]) - dt*sin(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]))/(pow(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]), 2) + pow(-(-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) + sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]), 2));
   out_7345596707252897319[44] = (dt*(sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*cos(dt*state[7])*cos(dt*state[8]) - dt*sin(dt*state[8])*sin(state[2])*cos(dt*state[7])*cos(state[1]))*(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]))/(pow(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]), 2) + pow(-(-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) + sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]), 2)) + (dt*(sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*cos(dt*state[7])*cos(dt*state[8]) - dt*sin(dt*state[8])*cos(dt*state[7])*cos(state[1])*cos(state[2]))*((-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) - (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) - sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]))/(pow(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]), 2) + pow(-(-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) + sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]), 2));
   out_7345596707252897319[45] = 0;
   out_7345596707252897319[46] = 0;
   out_7345596707252897319[47] = 0;
   out_7345596707252897319[48] = 0;
   out_7345596707252897319[49] = 0;
   out_7345596707252897319[50] = 0;
   out_7345596707252897319[51] = 0;
   out_7345596707252897319[52] = 0;
   out_7345596707252897319[53] = 0;
   out_7345596707252897319[54] = 0;
   out_7345596707252897319[55] = 0;
   out_7345596707252897319[56] = 0;
   out_7345596707252897319[57] = 1;
   out_7345596707252897319[58] = 0;
   out_7345596707252897319[59] = 0;
   out_7345596707252897319[60] = 0;
   out_7345596707252897319[61] = 0;
   out_7345596707252897319[62] = 0;
   out_7345596707252897319[63] = 0;
   out_7345596707252897319[64] = 0;
   out_7345596707252897319[65] = 0;
   out_7345596707252897319[66] = dt;
   out_7345596707252897319[67] = 0;
   out_7345596707252897319[68] = 0;
   out_7345596707252897319[69] = 0;
   out_7345596707252897319[70] = 0;
   out_7345596707252897319[71] = 0;
   out_7345596707252897319[72] = 0;
   out_7345596707252897319[73] = 0;
   out_7345596707252897319[74] = 0;
   out_7345596707252897319[75] = 0;
   out_7345596707252897319[76] = 1;
   out_7345596707252897319[77] = 0;
   out_7345596707252897319[78] = 0;
   out_7345596707252897319[79] = 0;
   out_7345596707252897319[80] = 0;
   out_7345596707252897319[81] = 0;
   out_7345596707252897319[82] = 0;
   out_7345596707252897319[83] = 0;
   out_7345596707252897319[84] = 0;
   out_7345596707252897319[85] = dt;
   out_7345596707252897319[86] = 0;
   out_7345596707252897319[87] = 0;
   out_7345596707252897319[88] = 0;
   out_7345596707252897319[89] = 0;
   out_7345596707252897319[90] = 0;
   out_7345596707252897319[91] = 0;
   out_7345596707252897319[92] = 0;
   out_7345596707252897319[93] = 0;
   out_7345596707252897319[94] = 0;
   out_7345596707252897319[95] = 1;
   out_7345596707252897319[96] = 0;
   out_7345596707252897319[97] = 0;
   out_7345596707252897319[98] = 0;
   out_7345596707252897319[99] = 0;
   out_7345596707252897319[100] = 0;
   out_7345596707252897319[101] = 0;
   out_7345596707252897319[102] = 0;
   out_7345596707252897319[103] = 0;
   out_7345596707252897319[104] = dt;
   out_7345596707252897319[105] = 0;
   out_7345596707252897319[106] = 0;
   out_7345596707252897319[107] = 0;
   out_7345596707252897319[108] = 0;
   out_7345596707252897319[109] = 0;
   out_7345596707252897319[110] = 0;
   out_7345596707252897319[111] = 0;
   out_7345596707252897319[112] = 0;
   out_7345596707252897319[113] = 0;
   out_7345596707252897319[114] = 1;
   out_7345596707252897319[115] = 0;
   out_7345596707252897319[116] = 0;
   out_7345596707252897319[117] = 0;
   out_7345596707252897319[118] = 0;
   out_7345596707252897319[119] = 0;
   out_7345596707252897319[120] = 0;
   out_7345596707252897319[121] = 0;
   out_7345596707252897319[122] = 0;
   out_7345596707252897319[123] = 0;
   out_7345596707252897319[124] = 0;
   out_7345596707252897319[125] = 0;
   out_7345596707252897319[126] = 0;
   out_7345596707252897319[127] = 0;
   out_7345596707252897319[128] = 0;
   out_7345596707252897319[129] = 0;
   out_7345596707252897319[130] = 0;
   out_7345596707252897319[131] = 0;
   out_7345596707252897319[132] = 0;
   out_7345596707252897319[133] = 1;
   out_7345596707252897319[134] = 0;
   out_7345596707252897319[135] = 0;
   out_7345596707252897319[136] = 0;
   out_7345596707252897319[137] = 0;
   out_7345596707252897319[138] = 0;
   out_7345596707252897319[139] = 0;
   out_7345596707252897319[140] = 0;
   out_7345596707252897319[141] = 0;
   out_7345596707252897319[142] = 0;
   out_7345596707252897319[143] = 0;
   out_7345596707252897319[144] = 0;
   out_7345596707252897319[145] = 0;
   out_7345596707252897319[146] = 0;
   out_7345596707252897319[147] = 0;
   out_7345596707252897319[148] = 0;
   out_7345596707252897319[149] = 0;
   out_7345596707252897319[150] = 0;
   out_7345596707252897319[151] = 0;
   out_7345596707252897319[152] = 1;
   out_7345596707252897319[153] = 0;
   out_7345596707252897319[154] = 0;
   out_7345596707252897319[155] = 0;
   out_7345596707252897319[156] = 0;
   out_7345596707252897319[157] = 0;
   out_7345596707252897319[158] = 0;
   out_7345596707252897319[159] = 0;
   out_7345596707252897319[160] = 0;
   out_7345596707252897319[161] = 0;
   out_7345596707252897319[162] = 0;
   out_7345596707252897319[163] = 0;
   out_7345596707252897319[164] = 0;
   out_7345596707252897319[165] = 0;
   out_7345596707252897319[166] = 0;
   out_7345596707252897319[167] = 0;
   out_7345596707252897319[168] = 0;
   out_7345596707252897319[169] = 0;
   out_7345596707252897319[170] = 0;
   out_7345596707252897319[171] = 1;
   out_7345596707252897319[172] = 0;
   out_7345596707252897319[173] = 0;
   out_7345596707252897319[174] = 0;
   out_7345596707252897319[175] = 0;
   out_7345596707252897319[176] = 0;
   out_7345596707252897319[177] = 0;
   out_7345596707252897319[178] = 0;
   out_7345596707252897319[179] = 0;
   out_7345596707252897319[180] = 0;
   out_7345596707252897319[181] = 0;
   out_7345596707252897319[182] = 0;
   out_7345596707252897319[183] = 0;
   out_7345596707252897319[184] = 0;
   out_7345596707252897319[185] = 0;
   out_7345596707252897319[186] = 0;
   out_7345596707252897319[187] = 0;
   out_7345596707252897319[188] = 0;
   out_7345596707252897319[189] = 0;
   out_7345596707252897319[190] = 1;
   out_7345596707252897319[191] = 0;
   out_7345596707252897319[192] = 0;
   out_7345596707252897319[193] = 0;
   out_7345596707252897319[194] = 0;
   out_7345596707252897319[195] = 0;
   out_7345596707252897319[196] = 0;
   out_7345596707252897319[197] = 0;
   out_7345596707252897319[198] = 0;
   out_7345596707252897319[199] = 0;
   out_7345596707252897319[200] = 0;
   out_7345596707252897319[201] = 0;
   out_7345596707252897319[202] = 0;
   out_7345596707252897319[203] = 0;
   out_7345596707252897319[204] = 0;
   out_7345596707252897319[205] = 0;
   out_7345596707252897319[206] = 0;
   out_7345596707252897319[207] = 0;
   out_7345596707252897319[208] = 0;
   out_7345596707252897319[209] = 1;
   out_7345596707252897319[210] = 0;
   out_7345596707252897319[211] = 0;
   out_7345596707252897319[212] = 0;
   out_7345596707252897319[213] = 0;
   out_7345596707252897319[214] = 0;
   out_7345596707252897319[215] = 0;
   out_7345596707252897319[216] = 0;
   out_7345596707252897319[217] = 0;
   out_7345596707252897319[218] = 0;
   out_7345596707252897319[219] = 0;
   out_7345596707252897319[220] = 0;
   out_7345596707252897319[221] = 0;
   out_7345596707252897319[222] = 0;
   out_7345596707252897319[223] = 0;
   out_7345596707252897319[224] = 0;
   out_7345596707252897319[225] = 0;
   out_7345596707252897319[226] = 0;
   out_7345596707252897319[227] = 0;
   out_7345596707252897319[228] = 1;
   out_7345596707252897319[229] = 0;
   out_7345596707252897319[230] = 0;
   out_7345596707252897319[231] = 0;
   out_7345596707252897319[232] = 0;
   out_7345596707252897319[233] = 0;
   out_7345596707252897319[234] = 0;
   out_7345596707252897319[235] = 0;
   out_7345596707252897319[236] = 0;
   out_7345596707252897319[237] = 0;
   out_7345596707252897319[238] = 0;
   out_7345596707252897319[239] = 0;
   out_7345596707252897319[240] = 0;
   out_7345596707252897319[241] = 0;
   out_7345596707252897319[242] = 0;
   out_7345596707252897319[243] = 0;
   out_7345596707252897319[244] = 0;
   out_7345596707252897319[245] = 0;
   out_7345596707252897319[246] = 0;
   out_7345596707252897319[247] = 1;
   out_7345596707252897319[248] = 0;
   out_7345596707252897319[249] = 0;
   out_7345596707252897319[250] = 0;
   out_7345596707252897319[251] = 0;
   out_7345596707252897319[252] = 0;
   out_7345596707252897319[253] = 0;
   out_7345596707252897319[254] = 0;
   out_7345596707252897319[255] = 0;
   out_7345596707252897319[256] = 0;
   out_7345596707252897319[257] = 0;
   out_7345596707252897319[258] = 0;
   out_7345596707252897319[259] = 0;
   out_7345596707252897319[260] = 0;
   out_7345596707252897319[261] = 0;
   out_7345596707252897319[262] = 0;
   out_7345596707252897319[263] = 0;
   out_7345596707252897319[264] = 0;
   out_7345596707252897319[265] = 0;
   out_7345596707252897319[266] = 1;
   out_7345596707252897319[267] = 0;
   out_7345596707252897319[268] = 0;
   out_7345596707252897319[269] = 0;
   out_7345596707252897319[270] = 0;
   out_7345596707252897319[271] = 0;
   out_7345596707252897319[272] = 0;
   out_7345596707252897319[273] = 0;
   out_7345596707252897319[274] = 0;
   out_7345596707252897319[275] = 0;
   out_7345596707252897319[276] = 0;
   out_7345596707252897319[277] = 0;
   out_7345596707252897319[278] = 0;
   out_7345596707252897319[279] = 0;
   out_7345596707252897319[280] = 0;
   out_7345596707252897319[281] = 0;
   out_7345596707252897319[282] = 0;
   out_7345596707252897319[283] = 0;
   out_7345596707252897319[284] = 0;
   out_7345596707252897319[285] = 1;
   out_7345596707252897319[286] = 0;
   out_7345596707252897319[287] = 0;
   out_7345596707252897319[288] = 0;
   out_7345596707252897319[289] = 0;
   out_7345596707252897319[290] = 0;
   out_7345596707252897319[291] = 0;
   out_7345596707252897319[292] = 0;
   out_7345596707252897319[293] = 0;
   out_7345596707252897319[294] = 0;
   out_7345596707252897319[295] = 0;
   out_7345596707252897319[296] = 0;
   out_7345596707252897319[297] = 0;
   out_7345596707252897319[298] = 0;
   out_7345596707252897319[299] = 0;
   out_7345596707252897319[300] = 0;
   out_7345596707252897319[301] = 0;
   out_7345596707252897319[302] = 0;
   out_7345596707252897319[303] = 0;
   out_7345596707252897319[304] = 1;
   out_7345596707252897319[305] = 0;
   out_7345596707252897319[306] = 0;
   out_7345596707252897319[307] = 0;
   out_7345596707252897319[308] = 0;
   out_7345596707252897319[309] = 0;
   out_7345596707252897319[310] = 0;
   out_7345596707252897319[311] = 0;
   out_7345596707252897319[312] = 0;
   out_7345596707252897319[313] = 0;
   out_7345596707252897319[314] = 0;
   out_7345596707252897319[315] = 0;
   out_7345596707252897319[316] = 0;
   out_7345596707252897319[317] = 0;
   out_7345596707252897319[318] = 0;
   out_7345596707252897319[319] = 0;
   out_7345596707252897319[320] = 0;
   out_7345596707252897319[321] = 0;
   out_7345596707252897319[322] = 0;
   out_7345596707252897319[323] = 1;
}
void h_4(double *state, double *unused, double *out_2234390001006855485) {
   out_2234390001006855485[0] = state[6] + state[9];
   out_2234390001006855485[1] = state[7] + state[10];
   out_2234390001006855485[2] = state[8] + state[11];
}
void H_4(double *state, double *unused, double *out_3723941941092748966) {
   out_3723941941092748966[0] = 0;
   out_3723941941092748966[1] = 0;
   out_3723941941092748966[2] = 0;
   out_3723941941092748966[3] = 0;
   out_3723941941092748966[4] = 0;
   out_3723941941092748966[5] = 0;
   out_3723941941092748966[6] = 1;
   out_3723941941092748966[7] = 0;
   out_3723941941092748966[8] = 0;
   out_3723941941092748966[9] = 1;
   out_3723941941092748966[10] = 0;
   out_3723941941092748966[11] = 0;
   out_3723941941092748966[12] = 0;
   out_3723941941092748966[13] = 0;
   out_3723941941092748966[14] = 0;
   out_3723941941092748966[15] = 0;
   out_3723941941092748966[16] = 0;
   out_3723941941092748966[17] = 0;
   out_3723941941092748966[18] = 0;
   out_3723941941092748966[19] = 0;
   out_3723941941092748966[20] = 0;
   out_3723941941092748966[21] = 0;
   out_3723941941092748966[22] = 0;
   out_3723941941092748966[23] = 0;
   out_3723941941092748966[24] = 0;
   out_3723941941092748966[25] = 1;
   out_3723941941092748966[26] = 0;
   out_3723941941092748966[27] = 0;
   out_3723941941092748966[28] = 1;
   out_3723941941092748966[29] = 0;
   out_3723941941092748966[30] = 0;
   out_3723941941092748966[31] = 0;
   out_3723941941092748966[32] = 0;
   out_3723941941092748966[33] = 0;
   out_3723941941092748966[34] = 0;
   out_3723941941092748966[35] = 0;
   out_3723941941092748966[36] = 0;
   out_3723941941092748966[37] = 0;
   out_3723941941092748966[38] = 0;
   out_3723941941092748966[39] = 0;
   out_3723941941092748966[40] = 0;
   out_3723941941092748966[41] = 0;
   out_3723941941092748966[42] = 0;
   out_3723941941092748966[43] = 0;
   out_3723941941092748966[44] = 1;
   out_3723941941092748966[45] = 0;
   out_3723941941092748966[46] = 0;
   out_3723941941092748966[47] = 1;
   out_3723941941092748966[48] = 0;
   out_3723941941092748966[49] = 0;
   out_3723941941092748966[50] = 0;
   out_3723941941092748966[51] = 0;
   out_3723941941092748966[52] = 0;
   out_3723941941092748966[53] = 0;
}
void h_10(double *state, double *unused, double *out_7730713563924800251) {
   out_7730713563924800251[0] = 9.8100000000000005*sin(state[1]) - state[4]*state[8] + state[5]*state[7] + state[12] + state[15];
   out_7730713563924800251[1] = -9.8100000000000005*sin(state[0])*cos(state[1]) + state[3]*state[8] - state[5]*state[6] + state[13] + state[16];
   out_7730713563924800251[2] = -9.8100000000000005*cos(state[0])*cos(state[1]) - state[3]*state[7] + state[4]*state[6] + state[14] + state[17];
}
void H_10(double *state, double *unused, double *out_8748160934080505697) {
   out_8748160934080505697[0] = 0;
   out_8748160934080505697[1] = 9.8100000000000005*cos(state[1]);
   out_8748160934080505697[2] = 0;
   out_8748160934080505697[3] = 0;
   out_8748160934080505697[4] = -state[8];
   out_8748160934080505697[5] = state[7];
   out_8748160934080505697[6] = 0;
   out_8748160934080505697[7] = state[5];
   out_8748160934080505697[8] = -state[4];
   out_8748160934080505697[9] = 0;
   out_8748160934080505697[10] = 0;
   out_8748160934080505697[11] = 0;
   out_8748160934080505697[12] = 1;
   out_8748160934080505697[13] = 0;
   out_8748160934080505697[14] = 0;
   out_8748160934080505697[15] = 1;
   out_8748160934080505697[16] = 0;
   out_8748160934080505697[17] = 0;
   out_8748160934080505697[18] = -9.8100000000000005*cos(state[0])*cos(state[1]);
   out_8748160934080505697[19] = 9.8100000000000005*sin(state[0])*sin(state[1]);
   out_8748160934080505697[20] = 0;
   out_8748160934080505697[21] = state[8];
   out_8748160934080505697[22] = 0;
   out_8748160934080505697[23] = -state[6];
   out_8748160934080505697[24] = -state[5];
   out_8748160934080505697[25] = 0;
   out_8748160934080505697[26] = state[3];
   out_8748160934080505697[27] = 0;
   out_8748160934080505697[28] = 0;
   out_8748160934080505697[29] = 0;
   out_8748160934080505697[30] = 0;
   out_8748160934080505697[31] = 1;
   out_8748160934080505697[32] = 0;
   out_8748160934080505697[33] = 0;
   out_8748160934080505697[34] = 1;
   out_8748160934080505697[35] = 0;
   out_8748160934080505697[36] = 9.8100000000000005*sin(state[0])*cos(state[1]);
   out_8748160934080505697[37] = 9.8100000000000005*sin(state[1])*cos(state[0]);
   out_8748160934080505697[38] = 0;
   out_8748160934080505697[39] = -state[7];
   out_8748160934080505697[40] = state[6];
   out_8748160934080505697[41] = 0;
   out_8748160934080505697[42] = state[4];
   out_8748160934080505697[43] = -state[3];
   out_8748160934080505697[44] = 0;
   out_8748160934080505697[45] = 0;
   out_8748160934080505697[46] = 0;
   out_8748160934080505697[47] = 0;
   out_8748160934080505697[48] = 0;
   out_8748160934080505697[49] = 0;
   out_8748160934080505697[50] = 1;
   out_8748160934080505697[51] = 0;
   out_8748160934080505697[52] = 0;
   out_8748160934080505697[53] = 1;
}
void h_13(double *state, double *unused, double *out_3777997688627648949) {
   out_3777997688627648949[0] = state[3];
   out_3777997688627648949[1] = state[4];
   out_3777997688627648949[2] = state[5];
}
void H_13(double *state, double *unused, double *out_3886689267223951963) {
   out_3886689267223951963[0] = 0;
   out_3886689267223951963[1] = 0;
   out_3886689267223951963[2] = 0;
   out_3886689267223951963[3] = 1;
   out_3886689267223951963[4] = 0;
   out_3886689267223951963[5] = 0;
   out_3886689267223951963[6] = 0;
   out_3886689267223951963[7] = 0;
   out_3886689267223951963[8] = 0;
   out_3886689267223951963[9] = 0;
   out_3886689267223951963[10] = 0;
   out_3886689267223951963[11] = 0;
   out_3886689267223951963[12] = 0;
   out_3886689267223951963[13] = 0;
   out_3886689267223951963[14] = 0;
   out_3886689267223951963[15] = 0;
   out_3886689267223951963[16] = 0;
   out_3886689267223951963[17] = 0;
   out_3886689267223951963[18] = 0;
   out_3886689267223951963[19] = 0;
   out_3886689267223951963[20] = 0;
   out_3886689267223951963[21] = 0;
   out_3886689267223951963[22] = 1;
   out_3886689267223951963[23] = 0;
   out_3886689267223951963[24] = 0;
   out_3886689267223951963[25] = 0;
   out_3886689267223951963[26] = 0;
   out_3886689267223951963[27] = 0;
   out_3886689267223951963[28] = 0;
   out_3886689267223951963[29] = 0;
   out_3886689267223951963[30] = 0;
   out_3886689267223951963[31] = 0;
   out_3886689267223951963[32] = 0;
   out_3886689267223951963[33] = 0;
   out_3886689267223951963[34] = 0;
   out_3886689267223951963[35] = 0;
   out_3886689267223951963[36] = 0;
   out_3886689267223951963[37] = 0;
   out_3886689267223951963[38] = 0;
   out_3886689267223951963[39] = 0;
   out_3886689267223951963[40] = 0;
   out_3886689267223951963[41] = 1;
   out_3886689267223951963[42] = 0;
   out_3886689267223951963[43] = 0;
   out_3886689267223951963[44] = 0;
   out_3886689267223951963[45] = 0;
   out_3886689267223951963[46] = 0;
   out_3886689267223951963[47] = 0;
   out_3886689267223951963[48] = 0;
   out_3886689267223951963[49] = 0;
   out_3886689267223951963[50] = 0;
   out_3886689267223951963[51] = 0;
   out_3886689267223951963[52] = 0;
   out_3886689267223951963[53] = 0;
}
void h_14(double *state, double *unused, double *out_131601925830122397) {
   out_131601925830122397[0] = state[6];
   out_131601925830122397[1] = state[7];
   out_131601925830122397[2] = state[8];
}
void H_14(double *state, double *unused, double *out_239298915246735563) {
   out_239298915246735563[0] = 0;
   out_239298915246735563[1] = 0;
   out_239298915246735563[2] = 0;
   out_239298915246735563[3] = 0;
   out_239298915246735563[4] = 0;
   out_239298915246735563[5] = 0;
   out_239298915246735563[6] = 1;
   out_239298915246735563[7] = 0;
   out_239298915246735563[8] = 0;
   out_239298915246735563[9] = 0;
   out_239298915246735563[10] = 0;
   out_239298915246735563[11] = 0;
   out_239298915246735563[12] = 0;
   out_239298915246735563[13] = 0;
   out_239298915246735563[14] = 0;
   out_239298915246735563[15] = 0;
   out_239298915246735563[16] = 0;
   out_239298915246735563[17] = 0;
   out_239298915246735563[18] = 0;
   out_239298915246735563[19] = 0;
   out_239298915246735563[20] = 0;
   out_239298915246735563[21] = 0;
   out_239298915246735563[22] = 0;
   out_239298915246735563[23] = 0;
   out_239298915246735563[24] = 0;
   out_239298915246735563[25] = 1;
   out_239298915246735563[26] = 0;
   out_239298915246735563[27] = 0;
   out_239298915246735563[28] = 0;
   out_239298915246735563[29] = 0;
   out_239298915246735563[30] = 0;
   out_239298915246735563[31] = 0;
   out_239298915246735563[32] = 0;
   out_239298915246735563[33] = 0;
   out_239298915246735563[34] = 0;
   out_239298915246735563[35] = 0;
   out_239298915246735563[36] = 0;
   out_239298915246735563[37] = 0;
   out_239298915246735563[38] = 0;
   out_239298915246735563[39] = 0;
   out_239298915246735563[40] = 0;
   out_239298915246735563[41] = 0;
   out_239298915246735563[42] = 0;
   out_239298915246735563[43] = 0;
   out_239298915246735563[44] = 1;
   out_239298915246735563[45] = 0;
   out_239298915246735563[46] = 0;
   out_239298915246735563[47] = 0;
   out_239298915246735563[48] = 0;
   out_239298915246735563[49] = 0;
   out_239298915246735563[50] = 0;
   out_239298915246735563[51] = 0;
   out_239298915246735563[52] = 0;
   out_239298915246735563[53] = 0;
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

void pose_update_4(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<3, 3, 0>(in_x, in_P, h_4, H_4, NULL, in_z, in_R, in_ea, MAHA_THRESH_4);
}
void pose_update_10(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<3, 3, 0>(in_x, in_P, h_10, H_10, NULL, in_z, in_R, in_ea, MAHA_THRESH_10);
}
void pose_update_13(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<3, 3, 0>(in_x, in_P, h_13, H_13, NULL, in_z, in_R, in_ea, MAHA_THRESH_13);
}
void pose_update_14(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<3, 3, 0>(in_x, in_P, h_14, H_14, NULL, in_z, in_R, in_ea, MAHA_THRESH_14);
}
void pose_err_fun(double *nom_x, double *delta_x, double *out_528651136918936599) {
  err_fun(nom_x, delta_x, out_528651136918936599);
}
void pose_inv_err_fun(double *nom_x, double *true_x, double *out_6367489553962069677) {
  inv_err_fun(nom_x, true_x, out_6367489553962069677);
}
void pose_H_mod_fun(double *state, double *out_449719862301799713) {
  H_mod_fun(state, out_449719862301799713);
}
void pose_f_fun(double *state, double dt, double *out_4643815075913585868) {
  f_fun(state,  dt, out_4643815075913585868);
}
void pose_F_fun(double *state, double dt, double *out_7345596707252897319) {
  F_fun(state,  dt, out_7345596707252897319);
}
void pose_h_4(double *state, double *unused, double *out_2234390001006855485) {
  h_4(state, unused, out_2234390001006855485);
}
void pose_H_4(double *state, double *unused, double *out_3723941941092748966) {
  H_4(state, unused, out_3723941941092748966);
}
void pose_h_10(double *state, double *unused, double *out_7730713563924800251) {
  h_10(state, unused, out_7730713563924800251);
}
void pose_H_10(double *state, double *unused, double *out_8748160934080505697) {
  H_10(state, unused, out_8748160934080505697);
}
void pose_h_13(double *state, double *unused, double *out_3777997688627648949) {
  h_13(state, unused, out_3777997688627648949);
}
void pose_H_13(double *state, double *unused, double *out_3886689267223951963) {
  H_13(state, unused, out_3886689267223951963);
}
void pose_h_14(double *state, double *unused, double *out_131601925830122397) {
  h_14(state, unused, out_131601925830122397);
}
void pose_H_14(double *state, double *unused, double *out_239298915246735563) {
  H_14(state, unused, out_239298915246735563);
}
void pose_predict(double *in_x, double *in_P, double *in_Q, double dt) {
  predict(in_x, in_P, in_Q, dt);
}
}

const EKF pose = {
  .name = "pose",
  .kinds = { 4, 10, 13, 14 },
  .feature_kinds = {  },
  .f_fun = pose_f_fun,
  .F_fun = pose_F_fun,
  .err_fun = pose_err_fun,
  .inv_err_fun = pose_inv_err_fun,
  .H_mod_fun = pose_H_mod_fun,
  .predict = pose_predict,
  .hs = {
    { 4, pose_h_4 },
    { 10, pose_h_10 },
    { 13, pose_h_13 },
    { 14, pose_h_14 },
  },
  .Hs = {
    { 4, pose_H_4 },
    { 10, pose_H_10 },
    { 13, pose_H_13 },
    { 14, pose_H_14 },
  },
  .updates = {
    { 4, pose_update_4 },
    { 10, pose_update_10 },
    { 13, pose_update_13 },
    { 14, pose_update_14 },
  },
  .Hes = {
  },
  .sets = {
  },
  .extra_routines = {
  },
};

ekf_lib_init(pose)
