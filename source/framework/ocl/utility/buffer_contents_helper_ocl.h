/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/enum/usm_memory_placement.h"
#include "framework/ocl/cl.h"
#include "framework/utility/buffer_contents_helper.h"

class BufferContentsHelperOcl : public BufferContentsHelper {
  public:
    static cl_int fillBuffer(cl_command_queue queue, cl_mem buffer, size_t bufferSize, BufferContents contents);
    static cl_int fillUsmBufferOrHostPtr(cl_command_queue queue, void *ptr, size_t ptrSize, UsmMemoryPlacement placement, BufferContents contents);

  private:
    static cl_int fillBufferWithRandomBytes(cl_command_queue queue, cl_mem buffer, size_t bufferSize);
    static cl_int fillBufferWithZeros(cl_command_queue queue, cl_mem buffer, size_t bufferSize);
    static cl_int fillBufferWithIncreasingBytes(cl_command_queue queue, cl_mem buffer, size_t bufferSize);

    static cl_int fillUsmBuffer(cl_command_queue queue, void *usmBuffer, size_t bufferSize, BufferContents contents);
    static cl_int fillUsmBufferWithRandomBytes(cl_command_queue queue, void *usmBuffer, size_t bufferSize);
    static cl_int fillUsmBufferWithZeros(cl_command_queue queue, void *usmBuffer, size_t bufferSize);
};
