/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/enum_argument.h"
#include "framework/enum/data_type.h"

struct DataTypeArgument : EnumArgument<DataTypeArgument, DataType> {
    using EnumArgument::EnumArgument;
    ThisType &operator=(EnumType newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    static constexpr const char *enumName = "data type";
    const static inline EnumType invalidEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[6] = {EnumType::Int32, EnumType::Int64, EnumType::Float, EnumType::Double, EnumType::CopyableObject, EnumType::Mixed};
    static constexpr const char *enumValuesNames[6] = {"Int32", "Int64", "Float", "Double", "CopyableObject", "Mixed"};
};
