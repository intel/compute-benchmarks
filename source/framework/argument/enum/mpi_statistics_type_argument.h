/*
 * Copyright (C) 2023-2026 Intel Corporation
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

    static constexpr const char *enumName = "MPI statistics type";
    const static inline EnumType invalidEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[3] = {EnumType::Avg, EnumType::Max, EnumType::Min};
    static constexpr const char *enumValuesNames[3] = {"Avg", "Max", "Min"};
};
