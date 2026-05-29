/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/enum_argument.h"
#include "framework/enum/mutable_command_flag.h"

struct MutableCommandFlagArgument : EnumArgument<MutableCommandFlagArgument, MutableCommandFlag> {
    using EnumArgument::EnumArgument;
    ThisType &operator=(EnumType newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    static constexpr const char *enumName = "mutable command flag";
    const static inline EnumType invalidEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[9] = {EnumType::All, EnumType::KernelArguments, EnumType::GroupCount, EnumType::GroupSize, EnumType::GlobalOffset, EnumType::SignalEvent, EnumType::WaitEvents, EnumType::KernelInstruction, EnumType::GraphArguments};
    static constexpr const char *enumValuesNames[9] = {"All", "KernelArguments", "GroupCount", "GroupSize", "GlobalOffset", "SignalEvent", "WaitEvents", "KernelInstruction", "GraphArguments"};
};
