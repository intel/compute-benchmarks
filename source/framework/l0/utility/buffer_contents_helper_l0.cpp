/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "buffer_contents_helper_l0.h"

#include <level_zero/zer_api.h>
#include <memory>
#include <type_traits>

using namespace L0;

ze_result_t BufferContentsHelperL0::fillBuffer(ze_device_handle_t device, ze_context_handle_t context, ze_command_queue_handle_t queue, uint32_t queueOrdinal, void *buffer, size_t bufferSize, BufferContents contents, bool useImmediate) {
    switch (contents) {
    case BufferContents::Zeros: {
        const uint8_t pattern[] = {0};
        return executeCommands(device, context, queue, queueOrdinal, useImmediate, [&](ze_command_list_handle_t cmdList) {
            return zeCommandListAppendMemoryFill(cmdList, buffer, pattern, sizeof(pattern), bufferSize, nullptr, 0, nullptr);
        });
    }
    case BufferContents::Random:
        return fillBufferWithData(device, context, queue, queueOrdinal, buffer, bufferSize, [bufferSize](uint8_t *data) { fillWithRandomBytes(data, bufferSize); }, useImmediate);
    default:
        FATAL_ERROR("Unknown buffer contents");
    }
}

ze_result_t BufferContentsHelperL0::fillBuffer(LevelZero &levelzero, void *buffer, size_t bufferSize, BufferContents contents, bool useImmediate) {
    return fillBuffer(levelzero.device, levelzero.context, levelzero.commandQueue,
                      levelzero.commandQueueDesc.ordinal, buffer, bufferSize, contents, useImmediate);
}

ze_result_t BufferContentsHelperL0::fillBufferWithData(LevelZero &levelzero, void *buffer, size_t bufferSize, const GenerateData &generateData, bool useImmediate) {
    return fillBufferWithData(levelzero.device, levelzero.context, levelzero.commandQueue,
                              levelzero.commandQueueDesc.ordinal, buffer, bufferSize, generateData, useImmediate);
}

ze_result_t BufferContentsHelperL0::fillBufferWithData(ze_device_handle_t device, ze_context_handle_t context, ze_command_queue_handle_t queue, uint32_t queueOrdinal, void *buffer, size_t bufferSize, const GenerateData &generateData, bool useImmediate) {
    void *staging{};
    ze_host_mem_alloc_desc_t desc{ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC};
    ZE_RESULT_SUCCESS_OR_RETURN(zeMemAllocHost(context, &desc, bufferSize, 0, &staging));
    const auto freeStaging = [context](void *allocation) { zeMemFree(context, allocation); };
    std::unique_ptr<void, decltype(freeStaging)> stagingGuard(staging, freeStaging);
    generateData(static_cast<uint8_t *>(staging));

    ZE_RESULT_SUCCESS_OR_RETURN(executeCommands(device, context, queue, queueOrdinal, useImmediate, [&](ze_command_list_handle_t cmdList) {
        return zeCommandListAppendMemoryCopy(cmdList, buffer, staging, bufferSize, nullptr, 0, nullptr);
    }));
    return zeMemFree(context, stagingGuard.release());
}

ze_result_t BufferContentsHelperL0::executeCommands(ze_device_handle_t device, ze_context_handle_t context, ze_command_queue_handle_t queue, uint32_t queueOrdinal, bool useImmediate, const AppendCommands &appendCommands) {
    ze_command_list_handle_t cmdList{};
    if (useImmediate) {
        ze_command_queue_desc_t commandQueueDesc = zeDefaultGPUImmediateCommandQueueDesc;
        commandQueueDesc.ordinal = queueOrdinal;
        ZE_RESULT_SUCCESS_OR_RETURN(zeCommandListCreateImmediate(context, device, &commandQueueDesc, &cmdList));
    } else {
        ze_command_list_desc_t cmdListDesc{ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC};
        cmdListDesc.commandQueueGroupOrdinal = queueOrdinal;
        ZE_RESULT_SUCCESS_OR_RETURN(zeCommandListCreate(context, device, &cmdListDesc, &cmdList));
    }
    const auto destroyCmdList = [](ze_command_list_handle_t commandList) { zeCommandListDestroy(commandList); };
    std::unique_ptr<std::remove_pointer_t<ze_command_list_handle_t>, decltype(destroyCmdList)> cmdListGuard(cmdList, destroyCmdList);

    ZE_RESULT_SUCCESS_OR_RETURN(appendCommands(cmdList));
    if (useImmediate) {
        ZE_RESULT_SUCCESS_OR_RETURN(zeCommandListHostSynchronize(cmdList, std::numeric_limits<uint64_t>::max()));
    } else {
        ZE_RESULT_SUCCESS_OR_RETURN(zeCommandListClose(cmdList));
        ZE_RESULT_SUCCESS_OR_RETURN(zeCommandQueueExecuteCommandLists(queue, 1, &cmdList, nullptr));
        ZE_RESULT_SUCCESS_OR_RETURN(zeCommandQueueSynchronize(queue, std::numeric_limits<uint64_t>::max()));
    }
    return zeCommandListDestroy(cmdListGuard.release());
}
