/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/enum/buffer_contents.h"

#include <cstring>
#include <random>

class BufferContentsHelper {
  public:
    static void fill(uint8_t *buffer, size_t size, BufferContents contents);
    static void fillWithZeros(uint8_t *buffer, size_t size);
    static void fillWithRandomBytes(uint8_t *buffer, size_t size);
    static void fillWithIncreasingBytes(uint8_t *buffer, size_t size);

  private:
    static thread_local std::mt19937 generator;
    static std::vector<uint8_t> cachedRandomData;

    static uint64_t randomOword();
};
