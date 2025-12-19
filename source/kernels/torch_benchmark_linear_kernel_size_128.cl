/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#define ARRAY_SIZE 128

// Kernel prepared for data_type == double

kernel void torch_benchmark_linear_kernel_size_128(__global double *out_data) {
    // prepare data
    double data[ARRAY_SIZE];

    data[0] = (double)1;
    data[1] = (double)1;
    data[2] = (double)1;
    data[3] = (double)1;
    data[4] = (double)1;
    data[5] = (double)1;
    data[6] = (double)1;
    data[7] = (double)1;
    data[8] = (double)1;
    data[9] = (double)1;
    data[10] = (double)1;
    data[11] = (double)1;
    data[12] = (double)1;
    data[13] = (double)1;
    data[14] = (double)1;
    data[15] = (double)1;
    data[16] = (double)1;
    data[17] = (double)1;
    data[18] = (double)1;
    data[19] = (double)1;
    data[20] = (double)1;
    data[21] = (double)1;
    data[22] = (double)1;
    data[23] = (double)1;
    data[24] = (double)1;
    data[25] = (double)1;
    data[26] = (double)1;
    data[27] = (double)1;
    data[28] = (double)1;
    data[29] = (double)1;
    data[30] = (double)1;
    data[31] = (double)1;
    data[32] = (double)1;
    data[33] = (double)1;
    data[34] = (double)1;
    data[35] = (double)1;
    data[36] = (double)1;
    data[37] = (double)1;
    data[38] = (double)1;
    data[39] = (double)1;
    data[40] = (double)1;
    data[41] = (double)1;
    data[42] = (double)1;
    data[43] = (double)1;
    data[44] = (double)1;
    data[45] = (double)1;
    data[46] = (double)1;
    data[47] = (double)1;
    data[48] = (double)1;
    data[49] = (double)1;
    data[50] = (double)1;
    data[51] = (double)1;
    data[52] = (double)1;
    data[53] = (double)1;
    data[54] = (double)1;
    data[55] = (double)1;
    data[56] = (double)1;
    data[57] = (double)1;
    data[58] = (double)1;
    data[59] = (double)1;
    data[60] = (double)1;
    data[61] = (double)1;
    data[62] = (double)1;
    data[63] = (double)1;
    data[64] = (double)1;
    data[65] = (double)1;
    data[66] = (double)1;
    data[67] = (double)1;
    data[68] = (double)1;
    data[69] = (double)1;
    data[70] = (double)1;
    data[71] = (double)1;
    data[72] = (double)1;
    data[73] = (double)1;
    data[74] = (double)1;
    data[75] = (double)1;
    data[76] = (double)1;
    data[77] = (double)1;
    data[78] = (double)1;
    data[79] = (double)1;
    data[80] = (double)1;
    data[81] = (double)1;
    data[82] = (double)1;
    data[83] = (double)1;
    data[84] = (double)1;
    data[85] = (double)1;
    data[86] = (double)1;
    data[87] = (double)1;
    data[88] = (double)1;
    data[89] = (double)1;
    data[90] = (double)1;
    data[91] = (double)1;
    data[92] = (double)1;
    data[93] = (double)1;
    data[94] = (double)1;
    data[95] = (double)1;
    data[96] = (double)1;
    data[97] = (double)1;
    data[98] = (double)1;
    data[99] = (double)1;
    data[100] = (double)1;
    data[101] = (double)1;
    data[102] = (double)1;
    data[103] = (double)1;
    data[104] = (double)1;
    data[105] = (double)1;
    data[106] = (double)1;
    data[107] = (double)1;
    data[108] = (double)1;
    data[109] = (double)1;
    data[110] = (double)1;
    data[111] = (double)1;
    data[112] = (double)1;
    data[113] = (double)1;
    data[114] = (double)1;
    data[115] = (double)1;
    data[116] = (double)1;
    data[117] = (double)1;
    data[118] = (double)1;
    data[119] = (double)1;
    data[120] = (double)1;
    data[121] = (double)1;
    data[122] = (double)1;
    data[123] = (double)1;
    data[124] = (double)1;
    data[125] = (double)1;
    data[126] = (double)1;
    data[127] = (double)1;

    // sum all data in out_data
    double sum = 0.0f;
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        sum += data[i];
    }
    out_data[0] = sum;
}
