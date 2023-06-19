/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef MATH_OPERATION
#error "Need a definition of MATH_OPERATION"
#endif

#ifndef DATATYPE
#error "Need a definition of DATATYPE"
#endif

#ifdef INT64_DIV

// Special handling for int64 div - revert value back to start value
// every 32 operations to keep operand in 64-bit boundaries.
#define MATH_OPERATION_128(address, otherArgument) \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    address += 0x7FFFFFFF7FFFFFFF;                 \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    address += 0x7FFFFFFF7FFFFFFF;                 \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    address += 0x7FFFFFFF7FFFFFFF;                 \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    address += 0x7FFFFFFF7FFFFFFF;

#else

#define MATH_OPERATION_128(address, otherArgument) \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);        \
    MATH_OPERATION(address, otherArgument);

#endif // INT64_DIV

__kernel void do_math_operation(__global DATATYPE *buffer, const DATATYPE otherArgument, const uint iterations) {
    __global DATATYPE *address = buffer + get_global_id(0);

    DATATYPE value = *address;
    for (uint i = 0; i < iterations; i++) {
        MATH_OPERATION_128(value, otherArgument);
    }

    *address = value;
}
