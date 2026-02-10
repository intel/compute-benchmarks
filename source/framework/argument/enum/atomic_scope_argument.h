/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/enum_argument.h"
#include "framework/enum/atomic_scope.h"

struct AtomicScopeArgument : EnumArgument<AtomicScopeArgument, AtomicScope> {
    using EnumArgument::EnumArgument;
    ThisType &operator=(EnumType newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    static constexpr const char *enumName = "atomic scope";
    const static inline EnumType invalidEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[2] = {EnumType::Workgroup, EnumType::Device};
    static constexpr const char *enumValuesNames[2] = {"Workgroup", "Device"};
};
