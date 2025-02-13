/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

enum class HostptrReuseMode {
    Unknown,

    Aligned4KB,
    Misaligned,
    Usm,
    Map,
};
