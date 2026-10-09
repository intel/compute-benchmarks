/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/l0/levelzero.h"
#include "framework/utility/buffer_contents_helper.h"

#include <functional>

class BufferContentsHelperL0 : public BufferContentsHelper {
  public:
    using GenerateData = std::function<void(uint8_t *)>;

    static ze_result_t fillBuffer(ze_device_handle_t device, ze_context_handle_t context, ze_command_queue_handle_t queue,
                                  uint32_t queueOrdinal, void *buffer, size_t bufferSize, BufferContents contents, bool useImmediate);

    static ze_result_t fillBuffer(L0::LevelZero &levelzero, void *buffer, size_t bufferSize, BufferContents contents, bool useImmediate);

    static ze_result_t fillBufferWithData(L0::LevelZero &levelzero, void *buffer, size_t bufferSize, const GenerateData &generateData, bool useImmediate);

  private:
    using AppendCommands = std::function<ze_result_t(ze_command_list_handle_t)>;

    static ze_result_t fillBufferWithData(ze_device_handle_t device, ze_context_handle_t context, ze_command_queue_handle_t queue,
                                          uint32_t queueOrdinal, void *buffer, size_t bufferSize, const GenerateData &generateData, bool useImmediate);

    static ze_result_t executeCommands(ze_device_handle_t device, ze_context_handle_t context, ze_command_queue_handle_t queue,
                                       uint32_t queueOrdinal, bool useImmediate, const AppendCommands &appendCommands);
};
