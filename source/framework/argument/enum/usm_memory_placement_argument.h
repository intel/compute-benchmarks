/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/enum_argument.h"
#include "framework/enum/usm_memory_placement.h"

struct UsmMemoryPlacementArgument : EnumArgument<UsmMemoryPlacementArgument, UsmMemoryPlacement> {
    using EnumArgument::EnumArgument;
    ThisType &operator=(EnumType newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    const static inline std::string enumName = "memory placement";
    const static inline EnumType invalidEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[10] = {EnumType::Device, EnumType::Host, EnumType::Shared, EnumType::NonUsmMapped, EnumType::NonUsmMisaligned, EnumType::NonUsm4KBAligned, EnumType::NonUsm2MBAligned, EnumType::NonUsmImportedMisaligned, EnumType::NonUsmImported4KBAligned, EnumType::NonUsmImported2MBAligned};
    const static inline EnumType limitedTargets[3] = {EnumType::Device, EnumType::Host, EnumType::NonUsm4KBAligned};
    const static inline EnumType deviceAndHost[2] = {EnumType::Device, EnumType::Host};
    const static inline std::string enumValuesNames[10] = {"Device", "Host", "Shared", "non-USM-mapped", "non-USMmisaligned", "non-USM4KBAligned", "non-USM2MBAligned", "non-USMmisaligned-imported", "non-USM4KBAligned-imported", "non-USM2MBAligned-imported"};
};
