/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/enum_argument.h"
#include "framework/enum/event_scope_argument.h"

struct EventScopeArgument : EnumArgument<EventScopeArgument, EventScope> {
    using EnumArgument::EnumArgument;
    ThisType &operator=(EnumType newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    const static inline std::string enumName = "event scope selection";
    const static inline EnumType invalidEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[4] = {EnumType::scopeSubDevice, EnumType::scopeDevice, EnumType::scopeHost, EnumType::none};
    const static inline std::string enumValuesNames[4] = {"subdevice", "device", "host", "none"};
};
