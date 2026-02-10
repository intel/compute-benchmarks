/*
 * Copyright (C) 2022-2026 Intel Corporation
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

    static constexpr const char *enumName = "Measure type of allocation";
    const static inline EnumType invalidEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[3] = {EnumType::Allocate, EnumType::Free, EnumType::Both};
    static constexpr const char *enumValuesNames[3] = {"Allocate", "Free", "Both"};
};
