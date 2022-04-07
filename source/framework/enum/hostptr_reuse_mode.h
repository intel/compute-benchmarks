/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

enum class HostptrReuseMode {
    Unknown,

    None,
    Usm,
    Map,
};
