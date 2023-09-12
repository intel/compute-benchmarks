/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/enum_argument.h"
#include "framework/enum/mpi_test.h"

struct MpiStatisticsTypeArgument : EnumArgument<MpiStatisticsTypeArgument, MpiStatisticsType> {
    using EnumArgument::EnumArgument;
    ThisType &operator=(EnumType newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    const static inline std::string enumName = "MPI statistics type";
    const static inline EnumType invalidEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[3] = {EnumType::Avg, EnumType::Max, EnumType::Min};
    const static inline std::string enumValuesNames[3] = {"Avg", "Max", "Min"};
};
