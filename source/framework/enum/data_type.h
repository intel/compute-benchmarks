/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/utility/error.h"

enum class DataType {
    Unknown,
    Int32,
    Float,
};

namespace DataTypeHelper {
constexpr inline size_t getSize(DataType dataType) {
    switch (dataType) {
    case DataType::Int32:
        return 4u;
    case DataType::Float:
        return sizeof(float);
    default:
        FATAL_ERROR("Unkown data type")
    }
}

inline std::string toOpenclC(DataType dataType) {
    switch (dataType) {
    case DataType::Float:
        return "float";
    case DataType::Int32:
        return "int";
    default:
        FATAL_ERROR("Unknown data type");
    }
}

inline std::string toExplicitAtomicOpenclC(DataType dataType) {
    switch (dataType) {
    case DataType::Float:
        return "atomic_float";
    case DataType::Int32:
        return "atomic_int";
    default:
        FATAL_ERROR("Unknown data type");
    }
}
} // namespace DataTypeHelper
