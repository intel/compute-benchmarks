/*
 * Copyright (C) 2022-2024 Intel Corporation
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
    const static inline EnumType enumValues[8] = {EnumType::Device, EnumType::Host, EnumType::Shared, EnumType::NonUsm, EnumType::NonUsmImported, EnumType::NonUsmMapped, EnumType::NonUsm2MBAligned, EnumType::NonUsmImported2MBAligned};
    const static inline EnumType limitedTargets[3] = {EnumType::Device, EnumType::Host, EnumType::NonUsm};
    const static inline EnumType deviceAndHost[2] = {EnumType::Device, EnumType::Host};
    const static inline std::string enumValuesNames[8] = {"Device", "Host", "Shared", "non-USM", "non-USM-imported", "non-USM-mapped", "non-usm2MBAligned", "non-USM2MBAligned-imported"};
};
