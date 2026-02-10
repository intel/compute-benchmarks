/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/enum_argument.h"
#include "framework/enum/kernel_submit_pattern.h"

struct KernelSubmitPatternArgument : EnumArgument<KernelSubmitPatternArgument, KernelSubmitPattern> {
    using EnumArgument::EnumArgument;
    ThisType &operator=(EnumType newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    static constexpr const char *enumName = "kernel submit pattern";
    const static inline EnumType invalidEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[3] = {EnumType::Single, EnumType::D2h_after_batch, EnumType::H2d_before_batch};
    static constexpr const char *enumValuesNames[3] = {"Single", "D2h_after_batch", "H2d_before_batch"};
};
