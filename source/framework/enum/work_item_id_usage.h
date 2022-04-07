/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

enum class WorkItemIdUsage {
    Unknown,
    None,
    Global,
    Local,
    AtomicPerWorkgroup
};
