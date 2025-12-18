/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/utility/error.h"

#include <cstddef>

// don't own the memory, just copy pointers
class CopyableObject {
  public:
    CopyableObject() : v1(float{}), p2(nullptr), p3(nullptr), size2_(0), size3_(0) {}

    CopyableObject(const float &val1, double *ptr2, int *ptr3, size_t size2, size_t size3)
        : v1(val1), p2(ptr2), p3(ptr3), size2_(size2), size3_(size3) {}

    CopyableObject(const CopyableObject &other)
        : v1(other.v1), p2(other.p2), p3(other.p3), size2_(other.size2_), size3_(other.size3_) {}

    ~CopyableObject() = default;

    CopyableObject &operator=(const CopyableObject &other) {
        if (this != &other) {
            v1 = other.v1;
            p2 = other.p2;
            p3 = other.p3;
            size2_ = other.size2_;
            size3_ = other.size3_;
        }
        return *this;
    }

    CopyableObject operator+(const CopyableObject &other) const {
        CopyableObject result(*this);
        result.v1 = v1 + other.v1;
        for (size_t i = 0; i < size2_; i++) {
            result.p2[i] = p2[i] + other.p2[i];
        }
        for (size_t i = 0; i < size3_; i++) {
            result.p3[i] = p3[i] + other.p3[i];
        }
        return result;
    }

    CopyableObject &operator+=(const CopyableObject &other) {
        v1 += other.v1;
        for (size_t i = 0; i < size2_; i++) {
            p2[i] += other.p2[i];
        }
        for (size_t i = 0; i < size3_; i++) {
            p3[i] += other.p3[i];
        }
        return *this;
    }

    CopyableObject operator-(const CopyableObject &other) const {
        CopyableObject result(*this);
        result.v1 = v1 - other.v1;
        for (size_t i = 0; i < size2_; i++) {
            result.p2[i] = p2[i] - other.p2[i];
        }
        for (size_t i = 0; i < size3_; i++) {
            result.p3[i] = p3[i] - other.p3[i];
        }
        return result;
    }

    CopyableObject &operator-=(const CopyableObject &other) {
        v1 -= other.v1;
        for (size_t i = 0; i < size2_; i++) {
            p2[i] -= other.p2[i];
        }
        for (size_t i = 0; i < size3_; i++) {
            p3[i] -= other.p3[i];
        }
        return *this;
    }

    CopyableObject operator*(const CopyableObject &other) const {
        CopyableObject result(*this);
        result.v1 = v1 - other.v1;
        for (size_t i = 0; i < size2_; i++) {
            result.p2[i] = p2[i] * other.p2[i];
        }
        for (size_t i = 0; i < size3_; i++) {
            result.p3[i] = p3[i] * other.p3[i];
        }
        return result;
    }

    CopyableObject &operator*=(const CopyableObject &other) {
        v1 -= other.v1;
        for (size_t i = 0; i < size2_; i++) {
            p2[i] *= other.p2[i];
        }
        for (size_t i = 0; i < size3_; i++) {
            p3[i] *= other.p3[i];
        }
        return *this;
    }

    float v1;
    double *p2;
    int *p3;
    size_t size2_;
    size_t size3_;
};

enum class DataType {
    Unknown,
    Int32,
    Int64,
    Float,
    Double,
    CopyableObject,
    Mixed,
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
    case DataType::Double:
        return sizeof(double);
    case DataType::CopyableObject:
        return sizeof(CopyableObject);
    case DataType::Mixed:
        FATAL_ERROR("Mixed data type does not have a single size");
    default:
        FATAL_ERROR("Unkown data type")
    }
}

inline std::string toOpenclC(DataType dataType) {
    switch (dataType) {
    case DataType::Float:
        return "float";
    case DataType::Double:
        return "double";
    case DataType::Int32:
        return "int";
    case DataType::Int64:
        return "long";
    case DataType::CopyableObject:
        FATAL_ERROR("CopyableObject does not have OpenCL C representation");
    case DataType::Mixed:
        FATAL_ERROR("Mixed data type does not have OpenCL C representation");
    default:
        FATAL_ERROR("Unknown data type");
    }
}

inline std::string toExplicitAtomicOpenclC(DataType dataType) {
    switch (dataType) {
    case DataType::Float:
        return "atomic_float";
    case DataType::Double:
        return "atomic_double";
    case DataType::Int32:
        return "atomic_int";
    case DataType::Int64:
        return "atomic_long";
    case DataType::CopyableObject:
        FATAL_ERROR("CopyableObject does not have OpenCL C representation");
    case DataType::Mixed:
        FATAL_ERROR("Mixed data type does not have OpenCL C representation");
    default:
        FATAL_ERROR("Unknown data type");
    }
}
} // namespace DataTypeHelper
