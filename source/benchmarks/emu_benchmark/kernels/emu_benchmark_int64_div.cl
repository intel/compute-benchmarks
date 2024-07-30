/*
 * Copyright (C) 2023-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

__kernel void int64_div(__global long *buffer, const long divisor, const uint iterations) {
    __global long *address = buffer + get_global_id(0);

    // To be sure int64 division is executed, don't let operand
    // to be reduced to int32 boundaries.
    long value = *address;
    for (uint i = 0; i < iterations; i++) {
        value = value / divisor;
        value = value / divisor;
        value = value / divisor;
        value = value / divisor;
        value = value / divisor;
        value = value / divisor;
        value = value / divisor;
        value = value / divisor;
        value = value / divisor;
        value = value / divisor;
        value = value / divisor;
        value = value / divisor;
        value = value / divisor;
        value = value / divisor;
        value = value / divisor;
        value = value / divisor;
        value = value / divisor;
        value = value / divisor;
        value = value / divisor;
        value = value / divisor;
        value = value / divisor;
        value = value / divisor;
        value = value / divisor;
        value = value / divisor;
        value = value / divisor;
        value = value / divisor;
        value = value / divisor;
        value = value / divisor;
        value = value / divisor;
        value = value / divisor;
        value = value / divisor;
        // starting value:              0x7FFFFFFFFFFFFFFE
        // after shifted right 31 bits: 0x00000000FFFFFFFF
        // revert back to starting value to not fall into 32-bit values
        value = value + 0x7FFFFFFEFFFFFFFF;
    }

    *address = value;
}
