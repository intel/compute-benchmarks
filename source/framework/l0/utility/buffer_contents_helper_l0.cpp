/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "buffer_contents_helper_l0.h"

#include <memory>

ze_result_t BufferContentsHelperL0::fillBuffer(ze_device_handle_t device, ze_context_handle_t context, ze_command_queue_handle_t queue, uint32_t queueOrdinal, void *buffer, size_t bufferSize, BufferContents contents, bool useImmediate) {
    ze_command_list_handle_t cmdList{};
    void *stagingAllocation{};

    if (useImmediate) {
        ze_command_queue_desc_t commandQueueDesc{ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC};
        commandQueueDesc.mode = ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS;
        commandQueueDesc.ordinal = queueOrdinal;
        ZE_RESULT_SUCCESS_OR_RETURN(zeCommandListCreateImmediate(context, device, &commandQueueDesc, &cmdList));
    } else {
        ze_command_list_desc_t cmdListDesc{ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC};
        cmdListDesc.commandQueueGroupOrdinal = queueOrdinal;
        ZE_RESULT_SUCCESS_OR_RETURN(zeCommandListCreate(context, device, &cmdListDesc, &cmdList));
    }

    ze_result_t retVal{ZE_RESULT_SUCCESS};
    switch (contents) {
    case BufferContents::Zeros:
        retVal = fillBufferWithZeros(cmdList, buffer, bufferSize);
        break;
    case BufferContents::Random:
        retVal = fillBufferWithRandomBytes(context, cmdList, buffer, bufferSize, stagingAllocation);
        break;
    default:
        FATAL_ERROR("Unknown buffer contents");
    }

    if (!useImmediate) {
        ZE_RESULT_SUCCESS_OR_RETURN(zeCommandListClose(cmdList));
        ZE_RESULT_SUCCESS_OR_RETURN(zeCommandQueueExecuteCommandLists(queue, 1, &cmdList, nullptr));
        ZE_RESULT_SUCCESS_OR_RETURN(zeCommandQueueSynchronize(queue, std::numeric_limits<uint64_t>::max()));
    }

    ZE_RESULT_SUCCESS_OR_RETURN(zeCommandListDestroy(cmdList));

    if (contents == BufferContents::Random) {
        ZE_RESULT_SUCCESS_OR_RETURN(zeMemFree(context, stagingAllocation));
    }

    return retVal;
}

ze_result_t BufferContentsHelperL0::fillBuffer(LevelZero &levelzero, void *buffer, size_t bufferSize, BufferContents contents, bool useImmediate) {
    return fillBuffer(levelzero.device, levelzero.context, levelzero.commandQueue,
                      levelzero.commandQueueDesc.ordinal, buffer, bufferSize, contents, useImmediate);
}

ze_result_t BufferContentsHelperL0::fillBufferWithRandomBytes(ze_context_handle_t context, ze_command_list_handle_t cmdList, void *buffer, size_t bufferSize, void *stagingAllocation) {
    // Create staging allocation
    ze_host_mem_alloc_desc_t desc{ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC};
    ZE_RESULT_SUCCESS_OR_RETURN(zeMemAllocHost(context, &desc, bufferSize, 0, &stagingAllocation));
    fillWithRandomBytes(static_cast<uint8_t *>(stagingAllocation), bufferSize);

    // Copy to destination allocation
    ZE_RESULT_SUCCESS_OR_RETURN(zeCommandListAppendMemoryCopy(cmdList, buffer, stagingAllocation, bufferSize, nullptr, 0, nullptr));

    return ZE_RESULT_SUCCESS;
}

ze_result_t BufferContentsHelperL0::fillBufferWithZeros(ze_command_list_handle_t cmdList, void *buffer, size_t bufferSize) {
    const size_t pattern[] = {0};
    ZE_RESULT_SUCCESS_OR_RETURN(zeCommandListAppendMemoryFill(cmdList, buffer, pattern, sizeof(pattern), bufferSize, nullptr, 0, nullptr));
    return ZE_RESULT_SUCCESS;
}
