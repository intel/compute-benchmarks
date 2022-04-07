/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "buffer_contents_helper_l0.h"

#include <memory>

ze_result_t BufferContentsHelperL0::fillBuffer(ze_device_handle_t device, ze_context_handle_t context, ze_command_queue_handle_t queue, uint32_t queueOrdinal, void *buffer, size_t bufferSize, BufferContents contents) {
    switch (contents) {
    case BufferContents::Zeros:
        return fillBufferWithZeros(device, context, queue, queueOrdinal, buffer, bufferSize);
    case BufferContents::Random:
        return fillBufferWithRandomBytes(device, context, queue, queueOrdinal, buffer, bufferSize);
    default:
        FATAL_ERROR("Unknown buffer contents");
    }
}

ze_result_t BufferContentsHelperL0::fillBuffer(LevelZero &levelzero, void *buffer, size_t bufferSize, BufferContents contents) {
    return fillBuffer(levelzero.device, levelzero.context, levelzero.commandQueue,
                      levelzero.commandQueueDesc.ordinal, buffer, bufferSize, contents);
}

ze_result_t BufferContentsHelperL0::fillBufferWithRandomBytes(ze_device_handle_t device, ze_context_handle_t context, ze_command_queue_handle_t queue, uint32_t queueOrdinal, void *buffer, size_t bufferSize) {
    // Create staging allocation
    ze_host_mem_alloc_desc_t desc{ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC};
    void *stagingAllocation;
    ZE_RESULT_SUCCESS_OR_RETURN(zeMemAllocHost(context, &desc, bufferSize, 0, &stagingAllocation));
    ZE_RESULT_SUCCESS_OR_RETURN(zeContextMakeMemoryResident(context, device, stagingAllocation, bufferSize));
    fillWithRandomBytes(static_cast<uint8_t *>(stagingAllocation), bufferSize);

    // Copy to destination allocation
    ze_command_list_desc_t cmdListDesc{ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC};
    cmdListDesc.commandQueueGroupOrdinal = queueOrdinal;
    ze_command_list_handle_t cmdList{};
    ZE_RESULT_SUCCESS_OR_RETURN(zeCommandListCreate(context, device, &cmdListDesc, &cmdList));
    ZE_RESULT_SUCCESS_OR_RETURN(zeCommandListAppendMemoryCopy(cmdList, buffer, stagingAllocation, bufferSize, nullptr, 0, nullptr));
    ZE_RESULT_SUCCESS_OR_RETURN(zeCommandListClose(cmdList));
    ZE_RESULT_SUCCESS_OR_RETURN(zeCommandQueueExecuteCommandLists(queue, 1, &cmdList, nullptr));
    ZE_RESULT_SUCCESS_OR_RETURN(zeCommandQueueSynchronize(queue, std::numeric_limits<uint64_t>::max()));
    ZE_RESULT_SUCCESS_OR_RETURN(zeCommandListDestroy(cmdList));

    // Destroy staging allocation
    ZE_RESULT_SUCCESS_OR_RETURN(zeContextEvictMemory(context, device, stagingAllocation, bufferSize));
    ZE_RESULT_SUCCESS_OR_RETURN(zeMemFree(context, stagingAllocation));

    return ZE_RESULT_SUCCESS;
}

ze_result_t BufferContentsHelperL0::fillBufferWithZeros(ze_device_handle_t device, ze_context_handle_t context, ze_command_queue_handle_t queue, uint32_t queueOrdinal, void *buffer, size_t bufferSize) {
    ze_command_list_desc_t cmdListDesc{ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC};
    cmdListDesc.commandQueueGroupOrdinal = queueOrdinal;
    ze_command_list_handle_t cmdList{};
    const size_t pattern[] = {0};
    ZE_RESULT_SUCCESS_OR_RETURN(zeCommandListCreate(context, device, &cmdListDesc, &cmdList));
    ZE_RESULT_SUCCESS_OR_RETURN(zeCommandListAppendMemoryFill(cmdList, buffer, pattern, sizeof(pattern), bufferSize, nullptr, 0, nullptr));
    ZE_RESULT_SUCCESS_OR_RETURN(zeCommandListClose(cmdList));
    ZE_RESULT_SUCCESS_OR_RETURN(zeCommandQueueExecuteCommandLists(queue, 1, &cmdList, nullptr));
    ZE_RESULT_SUCCESS_OR_RETURN(zeCommandQueueSynchronize(queue, std::numeric_limits<uint64_t>::max()));
    ZE_RESULT_SUCCESS_OR_RETURN(zeCommandListDestroy(cmdList));

    return ZE_RESULT_SUCCESS;
}
