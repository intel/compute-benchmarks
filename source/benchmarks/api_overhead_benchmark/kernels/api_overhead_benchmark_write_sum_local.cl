/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

__kernel void write_sum_local(__global int *outBuffer) {
   size_t x = get_local_id(0);
   size_t y = get_local_id(1);
   size_t z = get_local_id(2);
   outBuffer[0] = x + y + z;
}
