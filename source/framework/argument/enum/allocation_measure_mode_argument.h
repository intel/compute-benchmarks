/*
 * Copyright (C) 2022-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/enum_argument.h"
#include "framework/enum/allocation_measure_mode.h"

struct AllocationMeasureModeArgument : EnumArgument<AllocationMeasureModeArgument, AllocationMeasureMode> {
    using EnumArgument::EnumArgument;
    ThisType &operator=(EnumType newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    const static inline std::string enumName = "Measure type of allocation";
    const static inline EnumType invalidEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[3] = {EnumType::Allocate, EnumType::Free, EnumType::Both};
    const static inline std::string enumValuesNames[3] = {"Allocate", "Free", "Both"};
};
