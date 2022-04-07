/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/utility/error.h"

enum class UsmRuntimeMemoryPlacement {
    Unknown,
    Host,
    Device,
    Shared
};
