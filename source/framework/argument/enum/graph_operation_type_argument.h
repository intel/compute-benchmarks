/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/enum_argument.h"
#include "framework/enum/graph_operation_type.h"

struct GraphOperationTypeArgument : EnumArgument<GraphOperationTypeArgument, GraphOperationType> {
    using EnumArgument::EnumArgument;
    ThisType &operator=(EnumType newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    const static inline std::string enumName = "";
    const static inline EnumType invalidEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[3] = {EnumType::Init, EnumType::Mutate, EnumType::Execute};
    const static inline std::string enumValuesNames[3] = {"Initialize", "Mutate", "Execute"};
};
