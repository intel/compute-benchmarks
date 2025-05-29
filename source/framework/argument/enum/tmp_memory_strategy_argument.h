/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/enum_argument.h"
#include "framework/enum/tmp_memory_strategy.h"

struct TmpMemoryStrategyArgument : EnumArgument<TmpMemoryStrategyArgument, TmpMemoryStrategy> {
    using EnumArgument::EnumArgument;
    ThisType &operator=(EnumType newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    const static inline std::string enumName = "temporary memory allocation strategy";
    const static inline EnumType invalidEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[3] = {EnumType::Async, EnumType::Static, EnumType::Sync};
    const static inline std::string enumValuesNames[3] = {"async", "static", "sync"};
};
