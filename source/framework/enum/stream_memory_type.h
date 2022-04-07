/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/utility/error.h"

#include <string>

enum class StreamMemoryType {
    Unknown,
    Read,
    Write,
    Scale,
    Triad,
};