/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

enum class VerificationMode {
    Unknown,
    Light,    /// One element per 256 work groups
    Complete, /// Every element
};
