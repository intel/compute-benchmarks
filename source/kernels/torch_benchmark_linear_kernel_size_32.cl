/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#define ARRAY_SIZE 32

// Kernel prepared for data_type == double

kernel void torch_benchmark_linear_kernel_size_32(__global double *out_data) {
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

    // sum all data in out_data
    double sum = 0.0f;
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        sum += data[i];
    }
    out_data[0] = sum;
}
