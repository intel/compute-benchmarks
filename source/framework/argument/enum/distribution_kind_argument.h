/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/enum_argument.h"
#include "framework/enum/distribution_kind.h"

struct DistributionKindArgument : EnumArgument<DistributionKindArgument, DistributionKind> {
    using EnumArgument::EnumArgument;
    ThisType &operator=(EnumType newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    const static inline std::string enumName = "distribution kind";
    const static inline EnumType invalidEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[2] = {EnumType::Uniform, EnumType::LogUniform};
    const static inline std::string enumValuesNames[2] = {"Uniform", "LogUniform"};
};
