/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef MATH_OPERATION_PAIR
#error "Need a definition of MATH_OPERATION_PAIR"
#endif
#ifndef DATATYPE
#error "Need a definition of DATATYPE"
#endif

#define REPEAT_2(statement) statement statement
#define REPEAT_8(statement) REPEAT_2(REPEAT_2(REPEAT_2(statement)))
#define REPEAT_64(statement) REPEAT_8(REPEAT_8(statement))

__kernel void do_math_operation(__global DATATYPE *buffer, const DATATYPE otherArgument, const uint iterations) {
    __global DATATYPE *address = buffer + get_global_id(0);

    DATATYPE value = *address;
    DATATYPE other = otherArgument;
    for (uint i = 0; i < iterations; i++) {
        REPEAT_64(MATH_OPERATION_PAIR(value, other);)
    }

    *address = value;
}
