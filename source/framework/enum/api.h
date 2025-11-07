/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/utility/error.h"

#include "api_additional.h"

#include <string>

enum class Api {
    // Default, invalid value
    Unknown = 0,

    // Supported APIs
    OpenCL,
    L0,
    SYCL,
    SYCLPREVIEW,
    OMP,
    UR,
    OPT,

    // Special values
    COUNT,
    FIRST = OpenCL,
    LAST = OPT,
    All = 0xffff,
};

namespace std {
inline std::string to_string(Api api) {
    switch (api) {
    case Api::OpenCL:
        return "ocl";
    case Api::L0:
        return "l0";
    case Api::SYCL:
        return "sycl";
    case Api::SYCLPREVIEW:
        return "syclpreview";
    case Api::OMP:
        return "omp";
    case Api::UR:
        return "ur";
    default:
        return to_string_additional(api);
    }
}
} // namespace std

inline std::string getUserFriendlyApiName(Api api) {
    switch (api) {
    case Api::OpenCL:
        return "OpenCL";
    case Api::L0:
        return "LevelZero";
    case Api::SYCL:
        return "SYCL";
    case Api::SYCLPREVIEW:
        return "SYCL_PREVIEW";
    case Api::OMP:
        return "OpenMP";
    case Api::UR:
        return "UnifiedRuntime";
    default:
        return getUserFriendlyAdditionalApiName(api);
    }
}

inline Api parseApi(const std::string &value) {
    if (value == "ocl") {
        return Api::OpenCL;
    } else if (value == "l0") {
        return Api::L0;
    } else if (value == "all") {
        return Api::All;
    } else if (value == "sycl") {
        return Api::SYCL;
    } else if (value == "syclpreview") {
        return Api::SYCLPREVIEW;
    } else if (value == "omp") {
        return Api::OMP;
    } else if (value == "ur") {
        return Api::UR;
    } else {
        return parseAdditionalApi(value);
    }
}

inline bool validateApi(Api api) {
    switch (api) {
    case Api::OpenCL:
    case Api::L0:
    case Api::SYCL:
    case Api::OMP:
    case Api::UR:
    case Api::OPT:
        return true;
    default:
        return false;
    }
}
