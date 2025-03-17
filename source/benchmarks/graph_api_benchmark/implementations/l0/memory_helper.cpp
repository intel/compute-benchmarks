/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "implementations/l0/memory_helper.h"

#include "framework/utility/file_helper.h"
namespace mem_helper {
using DataFloatPtr = SinKernelGraphBase::DataFloatPtr;
DataFloatPtr allocDevice(std::shared_ptr<LevelZero> levelzero, uint32_t count) {
    void *deviceptr = nullptr;
    ze_device_mem_alloc_desc_t deviceAllocationDesc = {
        ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC};

    EXPECT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero->context, &deviceAllocationDesc,
                                              count * sizeof(float), 0, levelzero->device,
                                              &deviceptr));

    auto copied = levelzero;
    return DataFloatPtr(static_cast<float *>(deviceptr), [copied](float *ptr) {
        EXPECT_ZE_RESULT_SUCCESS(zeMemFree(copied->context, ptr));
    });
}

DataFloatPtr allocHost(std::shared_ptr<LevelZero> levelzero, uint32_t count) {
    void *hostptr = nullptr;
    ze_host_mem_alloc_desc_t hostAllocationDesc = {
        ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC};

    EXPECT_ZE_RESULT_SUCCESS(zeMemAllocHost(levelzero->context, &hostAllocationDesc,
                                            count * sizeof(float), 0, &hostptr));

    auto copied = levelzero;
    return DataFloatPtr(static_cast<float *>(hostptr), [copied](float *ptr) {
        EXPECT_ZE_RESULT_SUCCESS(zeMemFree(copied->context, ptr));
    });
}
TestResult loadKernel(std::shared_ptr<LevelZero> levelzero, std::string spirv_file_name, std::string kernel_name, ze_kernel_handle_t *kernel_out,
                      ze_module_handle_t *module_out) {
    auto spirvModule =
        FileHelper::loadBinaryFile(spirv_file_name);

    if (spirvModule.size() == 0) {
        return TestResult::KernelNotFound;
    }

    ze_module_desc_t moduleDesc{ZE_STRUCTURE_TYPE_MODULE_DESC};

    moduleDesc.format = ZE_MODULE_FORMAT_IL_SPIRV;

    moduleDesc.pInputModule = reinterpret_cast<const uint8_t *>(spirvModule.data());

    moduleDesc.inputSize = spirvModule.size();

    ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};

    kernelDesc.pKernelName = kernel_name.c_str();

    ASSERT_ZE_RESULT_SUCCESS(zeModuleCreate(levelzero->context, levelzero->device,
                                            &moduleDesc, module_out, nullptr));

    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(*module_out, &kernelDesc, kernel_out));
    return TestResult::Success;
}
} // namespace mem_helper