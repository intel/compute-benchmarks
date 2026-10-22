/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/enum/buffer_contents.h"
#include "framework/enum/stream_memory_type.h"
#include "framework/enum/verification_mode.h"
#include "framework/test_case/test_result.h"
#include "framework/utility/buffer_contents_helper.h"
#include "framework/utility/error.h"
#include "framework/utility/verification_helper.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace StreamMemoryHelper {

inline constexpr size_t dummyOutputSize = 16u;

enum class Verification {
    None,
    Light,
    Complete,
};

inline Verification getVerification(bool verify, VerificationMode mode) {
    if (!verify) {
        return Verification::None;
    }
    return mode == VerificationMode::Complete ? Verification::Complete : Verification::Light;
}

// One sample per work group measurably lowers the Read bandwidth, one per 256 does not.
inline constexpr size_t sampleGroupStride = 256u;

inline size_t getSampleCount(size_t elementCount, size_t groupSize) {
    return (elementCount / groupSize + sampleGroupStride - 1) / sampleGroupStride;
}

inline size_t getSampleElement(size_t sample, size_t groupSize, size_t multiplier) {
    return (sample * sampleGroupStride * groupSize + sample % groupSize) * multiplier;
}

inline size_t getSamplesSize(size_t bufferSize, size_t elementSize, size_t groupSize) {
    return getSampleCount(bufferSize / elementSize, groupSize) * elementSize;
}

inline std::string getSampleGroupStrideDefinition() {
    return std::to_string(sampleGroupStride);
}

template <typename ReadChunk>
inline auto getChunkedSamples(size_t elementSize, size_t samplesSize, ReadChunk readChunk) {
    return [=, chunk = static_cast<const uint8_t *>(nullptr), chunkOffset = size_t{0}, chunkLength = size_t{0}](size_t sample) mutable -> const uint8_t * {
        const size_t offset = sample * elementSize;
        if (chunk == nullptr || offset < chunkOffset || offset >= chunkOffset + chunkLength) {
            chunkOffset = offset / VerificationHelper::chunkSize * VerificationHelper::chunkSize;
            chunkLength = std::min(VerificationHelper::chunkSize, samplesSize - chunkOffset);
            chunk = readChunk(chunkOffset, chunkLength);
            if (chunk == nullptr) {
                return nullptr;
            }
        }
        return chunk + offset - chunkOffset;
    };
}

inline size_t getReadOutputSize(Verification verification, size_t bufferSize, size_t samplesSize) {
    switch (verification) {
    case Verification::Light:
        return samplesSize;
    case Verification::Complete:
        return bufferSize;
    default:
        return dummyOutputSize;
    }
}

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

struct WorkSize {
    size_t groupSize;
    size_t globalSize;
    size_t bufferSize;
};

// The stream kernels index elements with a 32-bit int.
inline bool fitsKernelIndexRange(size_t globalSize, size_t multiplier) {
    return globalSize * multiplier <= static_cast<size_t>(std::numeric_limits<int32_t>::max());
}

inline std::optional<WorkSize> getWorkSize(size_t size, size_t elementSize, size_t maxGroupSize) {
    const size_t elementCount = size / elementSize;
    const size_t groupSize = std::min(maxGroupSize, elementCount);
    if (groupSize == 0) {
        return std::nullopt;
    }
    const size_t globalSize = elementCount / groupSize * groupSize;
    return WorkSize{groupSize, globalSize, globalSize * elementSize};
}

struct Input {
    bool random = false;
    uint8_t fill = 0;
    bool inverted = false;

    void copy(uint8_t *destination, size_t offset, size_t size) const {
        if (this->random) {
            BufferContentsHelper::copyRandomBytes(destination, offset, size);
        } else {
            std::memset(destination, this->fill, size);
        }
        if (this->inverted) {
            std::transform(destination, destination + size, destination, [](uint8_t byte) { return static_cast<uint8_t>(~byte); });
        }
    }
};

inline Input getInput(BufferContents contents, size_t inputIndex) {
    FATAL_ERROR_IF(contents != BufferContents::Zeros && contents != BufferContents::Random, "Unsupported buffer contents");
    if (contents == BufferContents::Zeros) {
        return {};
    }
    return {true, 0u, inputIndex > 0};
}

class Verifier {
  public:
    Verifier(StreamMemoryType type, BufferContents contents, size_t size, size_t elementSize, size_t multiplier, uint32_t scalar)
        : type(type), contents(contents), elementSize(elementSize), elementCount(size / elementSize), multiplier(multiplier), scalar(scalar),
          firstInput(getInput(contents, 0)), secondInput(getInput(contents, 1)), outputVerifier(size, elementSize),
          scratch(type == StreamMemoryType::Triad ? std::min(size, VerificationHelper::chunkSize) : 0u), expectedSample(elementSize) {}

    void setInputs(const Input &firstContents, const Input &secondContents) {
        this->firstInput = firstContents;
        this->secondInput = secondContents;
    }

    template <typename ReadChunk>
    TestResult verify(ReadChunk &&readChunk) {
        const auto getExpectedChunk = [this](size_t offset, size_t length, uint8_t *destination) { this->getExpectedChunk(offset, length, destination); };
        return this->outputVerifier.verify(getExpectedChunk, readChunk);
    }

    template <typename GetSample>
    TestResult verifySamples(size_t groupSize, GetSample &&getSample) {
        const size_t sampleCount = getSampleCount(this->elementCount, groupSize);
        for (size_t sample = 0; sample < sampleCount; sample++) {
            const size_t element = getSampleElement(sample, groupSize, this->multiplier);
            if (element >= this->elementCount) {
                continue;
            }
            const uint8_t *output = getSample(sample);
            if (output == nullptr) {
                return TestResult::Error;
            }
            this->getExpectedChunk(element * this->elementSize, this->elementSize, this->expectedSample.data());
            if (std::memcmp(output, this->expectedSample.data(), this->elementSize) != 0) {
                VerificationHelper::reportMismatch(element);
                return TestResult::VerificationFail;
            }
        }
        return TestResult::Success;
    }

  private:
    static uint32_t loadValue(const uint8_t *data, size_t index) {
        uint32_t value{};
        std::memcpy(&value, data + index * sizeof(value), sizeof(value));
        return value;
    }

    static void storeValue(uint8_t *data, size_t index, uint32_t value) {
        std::memcpy(data + index * sizeof(value), &value, sizeof(value));
    }

    static void copyWriteRandomPattern(uint8_t *destination, size_t offset, size_t size) {
        for (size_t copied = 0; copied < size;) {
            const size_t patternOffset = (offset + copied) % sizeof(writeRandomPattern);
            const size_t length = std::min(sizeof(writeRandomPattern) - patternOffset, size - copied);
            std::memcpy(destination + copied, writeRandomPattern + patternOffset, length);
            copied += length;
        }
    }

    void getExpectedChunk(size_t offset, size_t length, uint8_t *destination) {
        const size_t valuesCount = length / sizeof(uint32_t);
        switch (this->type) {
        case StreamMemoryType::Read:
            this->firstInput.copy(destination, offset, length);
            break;
        case StreamMemoryType::Write:
            if (this->contents == BufferContents::Random) {
                copyWriteRandomPattern(destination, offset, length);
            } else {
                for (size_t i = 0; i < valuesCount; i++) {
                    storeValue(destination, i, this->scalar);
                }
            }
            break;
        case StreamMemoryType::Scale:
            this->firstInput.copy(destination, offset, length);
            for (size_t i = 0; i < valuesCount; i++) {
                storeValue(destination, i, loadValue(destination, i) * this->scalar);
            }
            break;
        case StreamMemoryType::Triad:
            this->firstInput.copy(destination, offset, length);
            this->secondInput.copy(this->scratch.data(), offset, length);
            for (size_t i = 0; i < valuesCount; i++) {
                storeValue(destination, i, loadValue(destination, i) + loadValue(this->scratch.data(), i) * this->scalar);
            }
            break;
        default:
            FATAL_ERROR("Unknown StreamMemoryType");
        }

        if (this->multiplier > 1) {
            for (size_t element = offset / this->elementSize; element < (offset + length) / this->elementSize; element++) {
                if (element % this->multiplier != 0) {
                    std::memset(destination + element * this->elementSize - offset, VerificationHelper::poisonByte, this->elementSize);
                }
            }
        }
    }

    const StreamMemoryType type;
    const BufferContents contents;
    const size_t elementSize;
    const size_t elementCount;
    const size_t multiplier;
    const uint32_t scalar;
    Input firstInput;
    Input secondInput;
    VerificationHelper::OutputVerifier outputVerifier;
    std::vector<uint8_t> scratch;
    std::vector<uint8_t> expectedSample;
};

} // namespace StreamMemoryHelper
