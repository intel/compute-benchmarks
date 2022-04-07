/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/utility/error.h"

#include <string>

enum class MeasurementType {
    Unknown,
    Cpu,
    Gpu,
};

namespace std {
inline std::string to_string(MeasurementType unit) {
    switch (unit) {
    case MeasurementType::Cpu:
        return "[CPU]";
    case MeasurementType::Gpu:
        return "[GPU]";
    default:
        FATAL_ERROR("Unknown measurement unit");
    }
}
} // namespace std
