/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

/// Defines the type of profiler used.
enum class ProfilerType {
    Unknown,
    Timer,      /// Timer class, representing wall time
    CpuCounter, /// CpuCounter class, representing CPU instructions retired
};
