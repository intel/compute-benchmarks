/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/enum/measurement_unit.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

struct VectorWidthArgument : PositiveIntegerArgument {
    using PositiveIntegerArgument::PositiveIntegerArgument;

    VectorWidthArgument &operator=(size_t newValue) {
        PositiveIntegerArgument::operator=(newValue);
        return *this;
    }

    bool validate() const override {
        return this->value == 1 || this->value == 2 || this->value == 4 || this->value == 8 || this->value == 16;
    }
};

struct MultiLaneVectorWidthArgument : VectorWidthArgument {
    using VectorWidthArgument::VectorWidthArgument;

    MultiLaneVectorWidthArgument &operator=(size_t newValue) {
        VectorWidthArgument::operator=(newValue);
        return *this;
    }

    bool validate() const override {
        return this->value != 1 && VectorWidthArgument::validate();
    }
};

namespace MadPerformance {

enum class DataType {
    Half,
    Float,
    Double,
    Int32,
};

inline constexpr size_t threadsPerEu = 8u;
inline constexpr size_t wavesCount = 4u;
inline constexpr size_t madsPerLoop = 128u;
inline constexpr size_t madsPerWorkItem = 32768u;
inline constexpr size_t operationsPerMad = 2u;
inline constexpr uint16_t halfOne = 0x3c00u;
inline constexpr uint16_t halfLargestConsecutiveInteger = 0x6800u;

inline const char *getVectorWidthHelp() {
    return "Number of lanes of the vector type the kernel computes on (1 or 2 or 4 or 8 or 16)";
}

inline const char *getMultiLaneVectorWidthHelp() {
    return "Number of lanes of the vector type the kernel computes on (2 or 4 or 8 or 16)";
}

inline size_t getScalarSize(DataType dataType) {
    switch (dataType) {
    case DataType::Half:
        return sizeof(uint16_t);
    case DataType::Double:
        return sizeof(double);
    case DataType::Float:
        return sizeof(float);
    default:
        return sizeof(int32_t);
    }
}

inline std::string getScalarTypeName(DataType dataType) {
    switch (dataType) {
    case DataType::Half:
        return "half";
    case DataType::Double:
        return "double";
    case DataType::Float:
        return "float";
    default:
        return "int";
    }
}

inline MeasurementUnit getMeasurementUnit(DataType dataType) {
    return dataType == DataType::Int32 ? MeasurementUnit::GigaIOPS : MeasurementUnit::GigaFLOPS;
}

inline uint32_t getLoopIterations(size_t vectorWidth) {
    return static_cast<uint32_t>(madsPerWorkItem / madsPerLoop / vectorWidth);
}

inline uint64_t getOperationsPerKernel(size_t workItemCount) {
    return static_cast<uint64_t>(workItemCount) * madsPerWorkItem * operationsPerMad;
}

inline std::string getBuildOptions(DataType dataType, size_t vectorWidth) {
    const std::string scalarType = getScalarTypeName(dataType);
    const std::string vectorType = vectorWidth == 1 ? scalarType : scalarType + std::to_string(vectorWidth);
    std::string options = "-DSCALAR_TYPE=" + scalarType + " -DVECTOR_TYPE=" + vectorType + " -DMADS_PER_LOOP=" + std::to_string(madsPerLoop);
    if (dataType == DataType::Int32) {
        options += " -DINTEGER_TYPE";
    }
    return options;
}

template <typename T>
std::vector<uint8_t> toBytes(T value) {
    std::vector<uint8_t> bytes(sizeof(T));
    std::memcpy(bytes.data(), &value, sizeof(T));
    return bytes;
}

inline std::vector<uint8_t> getOne(DataType dataType) {
    switch (dataType) {
    case DataType::Half:
        return toBytes(halfOne);
    case DataType::Double:
        return toBytes(1.0);
    case DataType::Float:
        return toBytes(1.0f);
    default:
        return toBytes(int32_t{1});
    }
}

inline std::vector<uint8_t> getExpectedValue(DataType dataType, size_t vectorWidth) {
    const size_t madsPerLane = madsPerWorkItem / vectorWidth;
    switch (dataType) {
    case DataType::Half:
        return toBytes(halfLargestConsecutiveInteger);
    case DataType::Double:
        return toBytes(static_cast<double>(madsPerLane));
    case DataType::Float:
        return toBytes(static_cast<float>(madsPerLane));
    default:
        return toBytes(static_cast<int32_t>(madsPerLane));
    }
}

inline bool verify(DataType dataType, size_t vectorWidth, const std::vector<uint8_t> &result) {
    const std::vector<uint8_t> expected = getExpectedValue(dataType, vectorWidth);
    for (size_t offset = 0; offset < result.size(); offset += expected.size()) {
        if (std::memcmp(result.data() + offset, expected.data(), expected.size()) != 0) {
            return false;
        }
    }
    return true;
}

} // namespace MadPerformance
