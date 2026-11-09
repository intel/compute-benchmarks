/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

__kernel void copy_bidirectional_stripes(__global uint4 *hostBuffer, __global uint4 *deviceBuffer) {
    const size_t index = get_global_id(0);
    if ((index / STRIPE_ELEMENTS) % 2 == 1) {
        hostBuffer[index] = deviceBuffer[index];
    } else {
        deviceBuffer[index] = hostBuffer[index];
    }
}
