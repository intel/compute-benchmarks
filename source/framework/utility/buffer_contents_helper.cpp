/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/utility/buffer_contents_helper.h"

#include "framework/utility/error.h"

#include <cstring>

std::mt19937 BufferContentsHelper::generator{std::random_device{}()};
std::vector<uint8_t> BufferContentsHelper::cachedRandomData{};

void BufferContentsHelper::fill(uint8_t *buffer, size_t size, BufferContents contents) {
    switch (contents) {
    case BufferContents::Zeros:
        return fillWithZeros(buffer, size);
    case BufferContents::Random:
        return fillWithRandomBytes(buffer, size);
    case BufferContents::IncreasingBytes:
        return fillWithIncreasingBytes(buffer, size);
    default:
        FATAL_ERROR("Unknown buffer contents");
    }
}

void BufferContentsHelper::fillWithZeros(uint8_t *buffer, size_t size) {
    std::memset(buffer, 0, size);
}

void BufferContentsHelper::fillWithRandomBytes(uint8_t *buffer, size_t size) {
    // We randomize OWORDS instead of BYTES, because it's faster. Size of the cachedRandomData must be
    // aligned to sizeof(OWORD), so we don't have to handle generation of the dangling bytes.
    const auto randomizationChunkSize = sizeof(uint64_t);
    const auto alignedSize = (size + (randomizationChunkSize - 1)) / randomizationChunkSize * randomizationChunkSize;

    // If requested size is greater than what we've cached, generate more data
    const auto oldSize = cachedRandomData.size();
    if (alignedSize > oldSize) {
        cachedRandomData.resize(alignedSize);

        const auto remainingSize = alignedSize - oldSize;
        const auto numOwords = remainingSize / sizeof(uint64_t);

        uint8_t *bufferIterator = cachedRandomData.data() + oldSize;
        for (auto i = 0u; i < numOwords; i++) {
            *reinterpret_cast<uint64_t *>(bufferIterator) = randomOword();
            bufferIterator += sizeof(uint64_t);
        }
    }

    // Copy cached data to desired buffer
    std::memcpy(buffer, cachedRandomData.data(), size);
}

void BufferContentsHelper::fillWithIncreasingBytes(uint8_t *buffer, size_t size) {
    for (auto index = 0u; index < size; index++) {
        buffer[index] = static_cast<uint8_t>(index);
    }
}

uint64_t BufferContentsHelper::randomOword() {
    const uint64_t result = std::uniform_int_distribution<uint64_t>{}(generator);
    return result;
}
