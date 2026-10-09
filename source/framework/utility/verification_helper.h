/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/test_case/test_result.h"
#include "framework/utility/memory_constants.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <vector>

namespace VerificationHelper {

inline constexpr size_t chunkSize = 64 * MemoryConstants::megaByte;
inline constexpr uint8_t poisonByte = 0xFF;

inline void reportMismatch(size_t element) {
    std::cerr << "Verification failed at element " << element << std::endl;
}

class OutputVerifier {
  public:
    OutputVerifier(const size_t size, const size_t elementSize)
        : size(size), elementSize(elementSize), expectedChunk(std::min(size, chunkSize)) {}

    template <typename GetExpectedChunk, typename ReadChunk>
    TestResult verify(GetExpectedChunk &&getExpectedChunk, ReadChunk &&readChunk) {
        for (size_t offset = 0; offset < this->size; offset += chunkSize) {
            const size_t length = std::min(chunkSize, this->size - offset);
            const uint8_t *output = readChunk(offset, length);
            if (output == nullptr) {
                return TestResult::Error;
            }
            getExpectedChunk(offset, length, this->expectedChunk.data());
            const std::span<const uint8_t> expectedBytes(this->expectedChunk.data(), length);
            const auto [expectedIt, outputIt] = std::ranges::mismatch(expectedBytes, std::span<const uint8_t>(output, length));
            if (expectedIt != expectedBytes.end()) {
                reportMismatch((offset + static_cast<size_t>(expectedIt - expectedBytes.begin())) / this->elementSize);
                return TestResult::VerificationFail;
            }
        }
        return TestResult::Success;
    }

  private:
    const size_t size;
    const size_t elementSize;
    std::vector<uint8_t> expectedChunk;
};

template <typename T, typename ReadChunk>
inline TestResult verifyUniformOutput(size_t size, T expected, ReadChunk &&readChunk) {
    const size_t count = size / sizeof(T);
    const size_t chunkCount = chunkSize / sizeof(T);
    for (size_t first = 0; first < count; first += chunkCount) {
        const size_t length = std::min(chunkCount, count - first);
        const T *output = static_cast<const T *>(readChunk(first * sizeof(T), length * sizeof(T)));
        if (output == nullptr) {
            return TestResult::Error;
        }
        const std::span<const T> values(output, length);
        const auto mismatch = std::ranges::find_if(values, [expected](T value) { return value != expected; });
        if (mismatch != values.end()) {
            reportMismatch(first + static_cast<size_t>(mismatch - values.begin()));
            return TestResult::VerificationFail;
        }
    }
    return TestResult::Success;
}

} // namespace VerificationHelper
