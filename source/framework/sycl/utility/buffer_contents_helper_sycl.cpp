/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/sycl/utility/buffer_contents_helper_sycl.h"

#include "framework/sycl/utility/usm_helper.h"
#include "framework/utility/error.h"

#include <memory>

namespace SYCL {

namespace {

// Device allocations are not writable from the host, so every fill goes through host USM.
void fillThroughStaging(Sycl &sycl, void *buffer, size_t bufferSize, const std::function<void(uint8_t *)> &generateData) {
    void *staging = UsmHelper::allocate(UsmMemoryPlacement::Host, sycl, bufferSize);
    FATAL_ERROR_IF(staging == nullptr, "Failed to allocate staging buffer");
    const auto deallocate = [&sycl](void *allocation) { UsmHelper::deallocate(UsmMemoryPlacement::Host, sycl, allocation); };
    std::unique_ptr<void, decltype(deallocate)> stagingGuard(staging, deallocate);

    generateData(static_cast<uint8_t *>(staging));
    sycl.queue.memcpy(buffer, staging, bufferSize).wait();
}

} // namespace

void BufferContentsHelperSycl::fillBuffer(Sycl &sycl, void *buffer, size_t bufferSize, BufferContents contents) {
    switch (contents) {
    case BufferContents::Zeros:
        fillBufferWithPattern(sycl, buffer, bufferSize, 0u);
        break;
    case BufferContents::Random:
        fillBufferWithRandomBytes(sycl, buffer, bufferSize);
        break;
    default:
        FATAL_ERROR("Unknown buffer contents");
    }
}

void BufferContentsHelperSycl::fillBufferWithPattern(Sycl &sycl, void *buffer, size_t bufferSize, uint8_t pattern) {
    sycl.queue.memset(buffer, pattern, bufferSize).wait();
}

void BufferContentsHelperSycl::fillBufferWithData(Sycl &sycl, void *buffer, size_t bufferSize, const std::function<void(uint8_t *)> &generateData) {
    fillThroughStaging(sycl, buffer, bufferSize, generateData);
}

void BufferContentsHelperSycl::copyToHost(Sycl &sycl, void *destination, const void *source, size_t size) {
    sycl.queue.memcpy(destination, source, size).wait();
}

void BufferContentsHelperSycl::fillBufferWithRandomBytes(Sycl &sycl, void *buffer, size_t bufferSize) {
    fillThroughStaging(sycl, buffer, bufferSize, [bufferSize](uint8_t *data) { fillWithRandomBytes(data, bufferSize); });
}

} // namespace SYCL
