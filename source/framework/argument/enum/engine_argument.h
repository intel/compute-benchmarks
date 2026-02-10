/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/enum_argument.h"
#include "framework/enum/engine.h"

struct EngineArgument : EnumArgument<EngineArgument, Engine> {
    using EnumArgument::EnumArgument;
    ThisType &operator=(EnumType newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    static constexpr const char *enumName = "engine selection";
    const static inline EnumType invalidEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[14] = {EnumType::Rcs, EnumType::Ccs0, EnumType::Ccs1, EnumType::Ccs2, EnumType::Ccs3, EnumType::Bcs, EnumType::Bcs1, EnumType::Bcs2, EnumType::Bcs3, EnumType::Bcs4, EnumType::Bcs5, EnumType::Bcs6, EnumType::Bcs7, EnumType::Bcs8};
    static constexpr const char *enumValuesNames[14] = {"RCS", "CCS0", "CCS1", "CCS2", "CCS3", "BCS", "BCS1", "BCS2", "BCS3", "BCS4", "BCS5", "BCS6", "BCS7", "BCS8"};
};
