/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/enum/buffer_contents.h"
#include "framework/enum/stream_memory_type.h"
#include "framework/test_case/test_result.h"
#include "framework/utility/buffer_contents_helper.h"
#include "framework/utility/error.h"
#include "framework/utility/verification_helper.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

namespace StreamMemoryVerification {

inline constexpr uint8_t outputPoison = 0xFF;

inline constexpr uint8_t writeRandomPattern[256] = {
    0xFF, 0xEE, 0xFE, 0x01, 0xE0, 0xD0, 0xF2, 0x0F, 0x1C, 0xF1, 0xEF, 0xE3,
    0xD3, 0xA1, 0xF0, 0x3E, 0xE1, 0x2E, 0xED, 0xF0, 0x99, 0xE2, 0x0E, 0x0E,
    0x20, 0xEF, 0x0D, 0xD4, 0xEF, 0xCB, 0x1F, 0xF2, 0x33, 0xFF, 0xFE, 0x01,
    0x13, 0xF5, 0x00, 0x2E, 0x0E, 0x22, 0x32, 0x2F, 0x00, 0x10, 0xFD, 0xF1,
    0x0D, 0xB1, 0x01, 0x0F, 0xDD, 0x4E, 0x0F, 0xDD, 0x4A, 0xE1, 0x2E, 0xD5,
    0x1E, 0xDE, 0x23, 0xE0, 0x30, 0xA0, 0x23, 0x22, 0x0F, 0x2C, 0x0E, 0x03,
    0x9E, 0x2D, 0xC0, 0x01, 0x04, 0x2F, 0x02, 0x36, 0x1F, 0xE6, 0xE1, 0xFA,
    0xF2, 0x10, 0xCD, 0xD5, 0x2E, 0x52, 0x0F, 0x00, 0x13, 0x34, 0x2E, 0xEF,
    0xCF, 0x01, 0x1E, 0x3C, 0x00, 0x22, 0x4C, 0x2B, 0xF9, 0x10, 0x4D, 0x0D,
    0x1E, 0x2F, 0x21, 0xC2, 0x13, 0xF1, 0x10, 0xAD, 0x01, 0xFF, 0x02, 0x6E,
    0xD3, 0xE4, 0xDC, 0x00, 0x0C, 0x03, 0x21, 0xF2, 0x53, 0x1E, 0x2C, 0xC2,
    0x11, 0xBF, 0xFF, 0xC5, 0xF0, 0xEF, 0x12, 0x7F, 0x00, 0x2D, 0x02, 0xF2,
    0x3D, 0xF4, 0xFE, 0x11, 0xC3, 0xD1, 0x00, 0x10, 0xEE, 0xFE, 0xEC, 0xA0,
    0xFB, 0x1F, 0xD0, 0xFE, 0x33, 0x4F, 0xFE, 0x20, 0x0F, 0x22, 0x22, 0xFD,
    0x13, 0x26, 0x0D, 0xF0, 0x52, 0x32, 0xF5, 0xDE, 0x02, 0x2E, 0xD1, 0x50,
    0xFD, 0x22, 0x30, 0x41, 0xE5, 0x4D, 0xE0, 0x70, 0x1F, 0xCF, 0xB0, 0x20,
    0x01, 0xDF, 0xB1, 0x00, 0x22, 0xF1, 0xFF, 0xF2, 0xE1, 0xE3, 0x03, 0xFD,
    0xF3, 0xDA, 0xE1, 0x2D, 0xF0, 0xFF, 0xF3, 0xED, 0x0D, 0xEC, 0x0E, 0xEC,
    0xFF, 0x0F, 0x04, 0xFD, 0x01, 0x20, 0x41, 0xEC, 0x2D, 0xEF, 0x3D, 0xF0,
    0xF0, 0x75, 0xCF, 0xD1, 0x11, 0x01, 0xE0, 0x01, 0xDD, 0x2F, 0xF1, 0xF1,
    0x2E, 0xDF, 0x32, 0xF3, 0x0B, 0x21, 0xCB, 0x32, 0xE3, 0x1F, 0xFF, 0x12,
    0xF2, 0xE3, 0xC0, 0xB0};

inline std::string getWriteRandomPatternDefinition() {
    std::ostringstream definition;
    definition << std::hex;
    for (size_t i = 0; i < sizeof(writeRandomPattern); i++) {
        definition << (i == 0 ? "0x" : ",0x") << static_cast<unsigned int>(writeRandomPattern[i]);
    }
    return definition.str();
}

struct Input {
    const uint8_t *bytes = nullptr;
    uint8_t fill = 0;
    bool inverted = false;

    void copy(uint8_t *destination, size_t offset, size_t size) const {
        if (this->bytes) {
            std::memcpy(destination, this->bytes + offset, size);
        } else {
            std::memset(destination, this->fill, size);
        }
        if (this->inverted) {
            std::transform(destination, destination + size, destination, [](uint8_t byte) { return static_cast<uint8_t>(~byte); });
        }
    }
};

inline Input getInput(BufferContents contents, size_t size, size_t inputIndex) {
    FATAL_ERROR_IF(contents != BufferContents::Zeros && contents != BufferContents::Random, "Unsupported buffer contents");
    if (contents == BufferContents::Zeros) {
        return {};
    }
    return {BufferContentsHelper::getRandomBytes(size), 0u, inputIndex > 0};
}

struct Expected {
    StreamMemoryType type;
    BufferContents contents;
    Input x;
    Input y;
    size_t elementSize;
    size_t multiplier;
    uint32_t scalar;
};

inline Expected getExpected(StreamMemoryType type, BufferContents contents, size_t size, size_t elementSize, size_t multiplier, uint32_t scalar) {
    const Input x = getInput(contents, size, 0);
    Input y = x;
    y.inverted = contents == BufferContents::Random;
    return {type, contents, x, y, elementSize, multiplier, scalar};
}

inline uint32_t loadValue(const uint8_t *data, size_t index) {
    uint32_t value{};
    std::memcpy(&value, data + index * sizeof(value), sizeof(value));
    return value;
}

inline void storeValue(uint8_t *data, size_t index, uint32_t value) {
    std::memcpy(data + index * sizeof(value), &value, sizeof(value));
}

inline void getExpectedChunk(const Expected &expected, size_t offset, size_t size, uint8_t *destination, uint8_t *scratch) {
    const size_t firstValue = offset / sizeof(uint32_t);
    const size_t valuesCount = size / sizeof(uint32_t);
    const size_t valuesPerElement = expected.elementSize / sizeof(uint32_t);
    expected.x.copy(destination, offset, size);

    switch (expected.type) {
    case StreamMemoryType::Read:
        break;
    case StreamMemoryType::Write:
        if (expected.contents == BufferContents::Random) {
            for (size_t i = 0; i < size; i++) {
                destination[i] = writeRandomPattern[(offset + i) % sizeof(writeRandomPattern)];
            }
        } else {
            for (size_t i = 0; i < valuesCount; i++) {
                storeValue(destination, i, ((firstValue + i) / valuesPerElement) % expected.multiplier == 0 ? expected.scalar : 0u);
            }
        }
        break;
    case StreamMemoryType::Scale:
        for (size_t i = 0; i < valuesCount; i++) {
            storeValue(destination, i, loadValue(destination, i) * expected.scalar);
        }
        break;
    case StreamMemoryType::Triad:
        expected.y.copy(scratch, offset, size);
        for (size_t i = 0; i < valuesCount; i++) {
            storeValue(destination, i, loadValue(destination, i) + loadValue(scratch, i) * expected.scalar);
        }
        break;
    default:
        FATAL_ERROR("Unknown StreamMemoryType");
    }
}

template <typename ReadChunk>
inline TestResult verifyOutput(const Expected &expected, size_t size, ReadChunk &&readChunk) {
    std::vector<uint8_t> scratch(expected.type == StreamMemoryType::Triad ? std::min(size, VerificationHelper::chunkSize) : 0u);
    const auto getChunk = [&](size_t offset, size_t length, uint8_t *destination) { getExpectedChunk(expected, offset, length, destination, scratch.data()); };
    return VerificationHelper::verifyOutput(size, expected.elementSize, getChunk, readChunk);
}

} // namespace StreamMemoryVerification
