/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/l0/levelzero.h"
#include "framework/utility/buffer_contents_helper.h"

class BufferContentsHelperL0 : public BufferContentsHelper {
  public:
    static ze_result_t fillBuffer(ze_device_handle_t device, ze_context_handle_t context, ze_command_queue_handle_t queue,
                                  uint32_t queueOrdinal, void *buffer, size_t bufferSize, BufferContents contents, bool useImmediate);

    static ze_result_t fillBuffer(LevelZero &levelzero, void *buffer, size_t bufferSize, BufferContents contents, bool useImmediate);

  private:
    static ze_result_t fillBufferWithRandomBytes(ze_context_handle_t context, ze_command_list_handle_t cmdList,
                                                 void *buffer, size_t bufferSize, void *stagingAllocation);

    static ze_result_t fillBufferWithZeros(ze_command_list_handle_t cmdList, void *buffer, size_t bufferSize);
};
