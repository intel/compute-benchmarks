/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/enum/mutable_command_flag.h"

#include <level_zero/ze_api.h>

namespace L0::MutableCommandHelper {
ze_mutable_command_exp_flags_t toMutableCommandFlags(MutableCommandFlag flag, ze_mutable_command_exp_flags_t supportedFlags);
} // namespace L0::MutableCommandHelper
