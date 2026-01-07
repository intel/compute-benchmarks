/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/enum_argument.h"
#include "framework/enum/usm_runtime_memory_placement.h"

struct UsmRuntimeMemoryPlacementArgument : EnumArgument<UsmRuntimeMemoryPlacementArgument, UsmRuntimeMemoryPlacement> {
    using EnumArgument::EnumArgument;
    ThisType &operator=(EnumType newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    const static inline std::string enumName = "memory placement";
    const static inline EnumType invalidEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[3] = {EnumType::Device, EnumType::Host, EnumType::Shared};
    const static inline EnumType deviceAndHost[2] = {EnumType::Device, EnumType::Host};
    const static inline std::string enumValuesNames[3] = {"Device", "Host", "Shared"};
};
