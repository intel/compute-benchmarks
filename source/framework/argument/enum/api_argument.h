/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/enum_argument.h"
#include "framework/enum/api.h"

struct ApiArgument : EnumArgument<ApiArgument, Api> {
    using EnumArgument::EnumArgument;
    ThisType &operator=(EnumType newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    static constexpr const char *enumName = "api";
    const static inline EnumType invalidEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[6] = {EnumType::OpenCL, EnumType::L0, EnumType::SYCL, EnumType::OMP, EnumType::OPT, EnumType::All};
    static constexpr const char *enumValuesNames[6] = {"ocl", "l0", "sycl", "omp", "opt", "all"};
};
