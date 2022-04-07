/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/enum_argument.h"
#include "framework/enum/work_item_id_usage.h"

struct WorkItemIdUsageArgument : EnumArgument<WorkItemIdUsageArgument, WorkItemIdUsage> {
    using EnumArgument::EnumArgument;
    ThisType &operator=(EnumType newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    const static inline std::string enumName = "work item id usage";
    const static inline EnumType invalidEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[4] = {EnumType::None, EnumType::Global, EnumType::Local, EnumType::AtomicPerWorkgroup};
    const static inline std::string enumValuesNames[4] = {"None", "Global", "Local", "AtomicPerWkg"};
};
