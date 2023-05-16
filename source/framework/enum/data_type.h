/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/utility/error.h"

enum class DataType {
    Unknown,
    Int32,
    Int64,
    Float,
};

namespace DataTypeHelper {
constexpr inline size_t getSize(DataType dataType) {
    switch (dataType) {
    case DataType::Int32:
        return 4u;
    case DataType::Int64:
        return 8u;
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
    case DataType::Int64:
        return "long";
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
    case DataType::Int64:
        return "atomic_long";
    default:
        FATAL_ERROR("Unknown data type");
    }
}
} // namespace DataTypeHelper
