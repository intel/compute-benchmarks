/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

__kernel void write_one(__global int *outBuffer) {
    outBuffer[0] = 1;
}
