/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

enum class ProfilerType {
    Unknown,
    Timer,      /// Timer class, representing wall time
    CpuCounter, /// CpuCounter class, representing CPU instructions retired
};
