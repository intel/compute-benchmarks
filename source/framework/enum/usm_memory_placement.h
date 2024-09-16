/*
 * Copyright (C) 2022-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/utility/error.h"

enum class UsmMemoryPlacement {
    Unknown,
    Host,
    Device,
    Shared,
    NonUsm,
    NonUsmImported,
    NonUsmMapped,
    NonUsm2MBAligned,
    NonUsmImported2MBAligned
};

inline constexpr bool requiresImport(UsmMemoryPlacement inputType) {
    if (inputType == UsmMemoryPlacement::NonUsmImported || inputType == UsmMemoryPlacement::NonUsmImported2MBAligned) {
        return true;
    }
    return false;
}