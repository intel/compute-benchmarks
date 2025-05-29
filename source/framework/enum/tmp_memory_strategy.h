/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

enum class TmpMemoryStrategy {
    Unknown,
    // Allocate memory using synchronous API, synchronize after kernel execution, and free memory.
    Sync,
    // Allocate memory using asynchronous API, and asynchronously free memory after kernel execution.
    Async,
    // Allocate memory once and reuse it for all kernels, free memory after all kernels are executed.
    Static,
};
