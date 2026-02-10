/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/enum_argument.h"
#include "framework/enum/kernel_name.h"

struct KernelNameArgument : EnumArgument<KernelNameArgument, KernelName> {
    using EnumArgument::EnumArgument;
    ThisType &operator=(EnumType newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    static constexpr const char *enumName = "kernel name";
    const static inline EnumType invalidEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[3] = {EnumType::Empty, EnumType::Add, EnumType::AddSequence};
    static constexpr const char *enumValuesNames[3] = {"Empty", "Add", "AddSequence"};
};
