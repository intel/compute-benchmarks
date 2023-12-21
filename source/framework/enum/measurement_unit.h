/*
 * Copyright (C) 2022-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/utility/error.h"

#include <string>

enum class MeasurementUnit {
    Unknown,
    Microseconds,
    Nanoseconds,
    GigabytesPerSecond,
    Latency,
    Percentage,
};

namespace std {
inline std::string to_string(MeasurementUnit unit) {
    switch (unit) {
    case MeasurementUnit::Nanoseconds:
        return "[ns]";
    case MeasurementUnit::Microseconds:
        return "[us]";
    case MeasurementUnit::GigabytesPerSecond:
        return "[GB/s]";
    case MeasurementUnit::Latency:
        return "[clk]";
    case MeasurementUnit::Percentage:
        return "[%]";
    default:
        FATAL_ERROR("Unknown measurement unit");
    }
}
} // namespace std
