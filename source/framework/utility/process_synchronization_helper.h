/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

struct ProcessSynchronizationHelper {
    // Arbitrary character used for signalling processes
    constexpr static inline char synchronizationChar = '$';
};
