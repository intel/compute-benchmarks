/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/utility/error.h"

enum class MutableCommandFlag {
    All,
    KernelArguments,
    GroupCount,
    GroupSize,
    GlobalOffset,
    SignalEvent,
    WaitEvents,
    KernelInstruction,
    GraphArguments,
    Unknown,
};