/*
 * Copyright (C) 2022 Intel Corporation
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

    const static inline std::string enumName = "data type";
    const static inline EnumType invalidEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[2] = {EnumType::Int32, EnumType::Float};
    const static inline std::string enumValuesNames[2] = {"Int32", "Float"};
};
