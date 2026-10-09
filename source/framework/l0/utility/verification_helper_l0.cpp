/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/utility/verification_helper_l0.h"

#include "framework/utility/verification_helper.h"

#include <algorithm>
#include <limits>

VerificationHelperL0::~VerificationHelperL0() {
    if (this->staging) {
        EXPECT_ZE_RESULT_SUCCESS(zeMemFree(this->levelzero.context, this->staging));
    }
    if (this->cmdList) {
        EXPECT_ZE_RESULT_SUCCESS(zeCommandListDestroy(this->cmdList));
    }
}

ze_result_t VerificationHelperL0::initialize(size_t outputSize) {
    ze_command_list_desc_t cmdListDesc{ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC};
    cmdListDesc.commandQueueGroupOrdinal = this->levelzero.commandQueueDesc.ordinal;
    ZE_RESULT_SUCCESS_OR_RETURN(zeCommandListCreate(this->levelzero.context, this->levelzero.device, &cmdListDesc, &this->cmdList));
    ze_host_mem_alloc_desc_t hostDesc{ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC};
    return zeMemAllocHost(this->levelzero.context, &hostDesc, std::min(outputSize, VerificationHelper::chunkSize), 0, &this->staging);
}

ze_result_t VerificationHelperL0::poison(void *buffer, size_t size) {
    ZE_RESULT_SUCCESS_OR_RETURN(zeCommandListReset(this->cmdList));
    ZE_RESULT_SUCCESS_OR_RETURN(zeCommandListAppendMemoryFill(this->cmdList, buffer, &VerificationHelper::poisonByte, sizeof(VerificationHelper::poisonByte), size, nullptr, 0, nullptr));
    return this->execute();
}

const uint8_t *VerificationHelperL0::read(const void *buffer, size_t offset, size_t size) {
    ZE_RESULT_SUCCESS_OR_RETURN_VALUE(zeCommandListReset(this->cmdList), nullptr);
    ZE_RESULT_SUCCESS_OR_RETURN_VALUE(zeCommandListAppendMemoryCopy(this->cmdList, this->staging, static_cast<const uint8_t *>(buffer) + offset, size, nullptr, 0, nullptr), nullptr);
    ZE_RESULT_SUCCESS_OR_RETURN_VALUE(this->execute(), nullptr);
    return static_cast<const uint8_t *>(this->staging);
}

ze_result_t VerificationHelperL0::execute() {
    ZE_RESULT_SUCCESS_OR_RETURN(zeCommandListClose(this->cmdList));
    ZE_RESULT_SUCCESS_OR_RETURN(zeCommandQueueExecuteCommandLists(this->levelzero.commandQueue, 1, &this->cmdList, nullptr));
    return zeCommandQueueSynchronize(this->levelzero.commandQueue, std::numeric_limits<uint64_t>::max());
}
