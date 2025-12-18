/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

enum class KernelSubmitPattern {
    Unknown,
    Single,
    D2h_after_batch,
    H2d_before_batch,
};
