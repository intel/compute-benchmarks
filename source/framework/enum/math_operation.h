/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <sstream>
#include <string>

enum class MathOperation {
    Unknown,
    Add,
    Sub,
    Xchg,
    CmpXchg,
    Inc,
    Dec,
    Min,
    Max,
    And,
    Or,
    Xor,
    Div,
    Modulo,
};
