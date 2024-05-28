/*
 * Copyright (C) 2022-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/utility/error.h"

enum class AllocationMeasureMode {
    Unknown,
    Allocate,
    Free,
    Both,
};
