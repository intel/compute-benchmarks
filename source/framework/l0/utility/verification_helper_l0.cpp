/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/utility/verification_helper_l0.h"

#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/utility/verification_helper.h"

#include <algorithm>
#include <limits>

VerificationHelperL0::~VerificationHelperL0() {
    for (ze_kernel_handle_t kernel : {this->gatherKernel, this->resetKernel}) {
        if (kernel) {
            EXPECT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
        }
    }
    if (this->samplingModule) {
        EXPECT_ZE_RESULT_SUCCESS(zeModuleDestroy(this->samplingModule));
    }
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

TestResult VerificationHelperL0::initializeSampling(const char *programName, const char *buildOptions, void *output, void *samples,
                                                    size_t sampleCount, size_t elementCount, size_t groupSize, size_t multiplier) {
    if (const TestResult result = L0::KernelHelper::loadKernel(this->levelzero, programName, "gatherSamples", &this->gatherKernel, &this->samplingModule, buildOptions); result != TestResult::Success) {
        return result;
    }
    ze_kernel_desc_t resetKernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
    resetKernelDesc.pKernelName = "poisonSamples";
    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(this->samplingModule, &resetKernelDesc, &this->resetKernel));

    const uint32_t workItemCount = static_cast<uint32_t>(sampleCount);
    uint32_t samplingGroupSize[3] = {};
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSuggestGroupSize(this->gatherKernel, workItemCount, 1u, 1u, &samplingGroupSize[0], &samplingGroupSize[1], &samplingGroupSize[2]));
    this->samplingGroupCount = {(workItemCount + samplingGroupSize[0] - 1) / samplingGroupSize[0], 1u, 1u};
    const int32_t elementCountArgument = static_cast<int32_t>(elementCount);
    const int32_t groupSizeArgument = static_cast<int32_t>(groupSize);
    const int32_t multiplierArgument = static_cast<int32_t>(multiplier);
    for (ze_kernel_handle_t kernel : {this->gatherKernel, this->resetKernel}) {
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, samplingGroupSize[0], 1u, 1u));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(output), &output));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 1, sizeof(samples), &samples));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 2, sizeof(elementCountArgument), &elementCountArgument));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 3, sizeof(groupSizeArgument), &groupSizeArgument));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 4, sizeof(multiplierArgument), &multiplierArgument));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(this->resetKernel, 5, sizeof(VerificationHelper::poisonByte), &VerificationHelper::poisonByte));
    return TestResult::Success;
}

ze_result_t VerificationHelperL0::resetSamples() {
    return this->launch(this->resetKernel);
}

ze_result_t VerificationHelperL0::gather() {
    return this->launch(this->gatherKernel);
}

ze_result_t VerificationHelperL0::launch(ze_kernel_handle_t kernel) {
    ZE_RESULT_SUCCESS_OR_RETURN(zeCommandListReset(this->cmdList));
    ZE_RESULT_SUCCESS_OR_RETURN(zeCommandListAppendLaunchKernel(this->cmdList, kernel, &this->samplingGroupCount, nullptr, 0, nullptr));
    return this->execute();
}

ze_result_t VerificationHelperL0::execute() {
    ZE_RESULT_SUCCESS_OR_RETURN(zeCommandListClose(this->cmdList));
    ZE_RESULT_SUCCESS_OR_RETURN(zeCommandQueueExecuteCommandLists(this->levelzero.commandQueue, 1, &this->cmdList, nullptr));
    return zeCommandQueueSynchronize(this->levelzero.commandQueue, std::numeric_limits<uint64_t>::max());
}
