/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/utility/error.h"

struct BitHelper {
    static uint64_t isolateLowerNBits(const uint64_t value, const uint64_t bitsToIsolate) {
        if (bitsToIsolate >= 64) {
            return value;
        }
        uint64_t mask;
        mask = (1ull << bitsToIsolate) - 1;
        return value & mask;
    }
};
