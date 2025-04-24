/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

__kernel void write_one(__global int *outBuffer) {
    outBuffer[0] = 1;
}

enum LSC_STCC {
    LSC_STCC_DEFAULT   = 0,
    LSC_STCC_L1UC_L3UC = 1
};
void  __builtin_IB_lsc_store_global_uint  (__global uint *base, int immElemOff, uint val, enum LSC_STCC cacheOpt);

__kernel void write_one_uncached(__global uint *outBuffer) {
    __builtin_IB_lsc_store_global_uint(outBuffer, 0, 1, LSC_STCC_L1UC_L3UC);
}