/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/enum/buffer_contents.h"
#include "framework/sycl/sycl.h"
#include "framework/utility/buffer_contents_helper.h"

#include <functional>

namespace SYCL {

struct BufferContentsHelperSycl : BufferContentsHelper {
    static void fillBuffer(Sycl &sycl, void *buffer, size_t bufferSize, BufferContents contents);
    static void fillBufferWithPattern(Sycl &sycl, void *buffer, size_t bufferSize, uint8_t pattern);
    static void fillBufferWithData(Sycl &sycl, void *buffer, size_t bufferSize, const std::function<void(uint8_t *)> &generateData);
    static void copyToHost(Sycl &sycl, void *destination, const void *source, size_t size);

  private:
    static void fillBufferWithRandomBytes(Sycl &sycl, void *buffer, size_t bufferSize);
};

} // namespace SYCL
