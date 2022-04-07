/*
 * Copyright (C) 2022 Intel Corporation
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
    const static inline EnumType enumValues[6] = {EnumType::Device, EnumType::Host, EnumType::Shared, EnumType::NonUsm, EnumType::NonUsmImported, EnumType::NonUsmMapped};
    const static inline std::string enumValuesNames[6] = {"Device", "Host", "Shared", "non-USM", "non-USM-imported", "non-USM-mapped"};
};
