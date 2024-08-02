/*
 * Copyright (C) 2022-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/utility/error.h"

#include <string>

enum class Api {
    // Default, invalid value
    Unknown = 0,

    // Supported APIs
    OpenCL,
    L0,
    SYCL,
    OMP,
    UR,

    // Special values
    COUNT,
    FIRST = OpenCL,
    LAST = UR,
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
    case Api::OMP:
        return "omp";
    case Api::UR:
        return "ur";
    default:
        FATAL_ERROR("Unknown API");
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
    case Api::OMP:
        return "OpenMP";
    case Api::UR:
        return "UnifiedRuntime";
    default:
        FATAL_ERROR("Unknown API");
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
    } else if (value == "omp") {
        return Api::OMP;
    } else if (value == "ur") {
        return Api::UR;
    } else {
        return Api::Unknown;
    }
}

inline bool validateApi(Api api) {
    switch (api) {
    case Api::OpenCL:
    case Api::L0:
    case Api::SYCL:
    case Api::OMP:
    case Api::UR:
        return true;
    default:
        return false;
    }
}
