/*
 * Copyright (C) 2022-2025 Intel Corporation
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
    NonUsmMapped,
    NonUsmMisaligned,
    NonUsm4KBAligned,
    NonUsm2MBAligned,
    NonUsmImportedMisaligned,
    NonUsmImported4KBAligned,
    NonUsmImported2MBAligned,
};

inline constexpr bool requiresImport(UsmMemoryPlacement inputType) {
    if (inputType == UsmMemoryPlacement::NonUsmImportedMisaligned ||
        inputType == UsmMemoryPlacement::NonUsmImported4KBAligned ||
        inputType == UsmMemoryPlacement::NonUsmImported2MBAligned) {
        return true;
    }
    return false;
}

inline constexpr bool isSharedSystemPointer(UsmMemoryPlacement inputType) {
    if (inputType == UsmMemoryPlacement::NonUsm ||
        inputType == UsmMemoryPlacement::NonUsmMisaligned ||
        inputType == UsmMemoryPlacement::NonUsm4KBAligned ||
        inputType == UsmMemoryPlacement::NonUsm2MBAligned) {
        return true;
    }
    return false;
}

inline constexpr bool isUsmMemoryType(UsmMemoryPlacement inputType) {
    switch (inputType) {
    case UsmMemoryPlacement::Host:
    case UsmMemoryPlacement::Device:
    case UsmMemoryPlacement::Shared:
    case UsmMemoryPlacement::NonUsmImportedMisaligned:
    case UsmMemoryPlacement::NonUsmImported4KBAligned:
    case UsmMemoryPlacement::NonUsmImported2MBAligned:
        return true;
    default:
        return false;
    }
}