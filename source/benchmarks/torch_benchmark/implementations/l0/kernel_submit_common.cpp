/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "kernel_submit_common.hpp"

TestResult init_level_zero_common(L0CommonContext &ctx) {
    ASSERT_ZE_RESULT_SUCCESS(zeInit(ZE_INIT_FLAG_GPU_ONLY));

    // driver + device
    uint32_t driverCount = 0;
    ASSERT_ZE_RESULT_SUCCESS(zeDriverGet(&driverCount, nullptr));
    std::vector<ze_driver_handle_t> drivers(driverCount);
    ASSERT_ZE_RESULT_SUCCESS(zeDriverGet(&driverCount, drivers.data()));
    ze_driver_handle_t driver = drivers[0];

    uint32_t deviceCount = 0;
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGet(driver, &deviceCount, nullptr));
    std::vector<ze_device_handle_t> devices(deviceCount);
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGet(driver, &deviceCount, devices.data()));
    ze_device_handle_t device = devices[0];

    // context
    ze_context_handle_t context = zeDriverGetDefaultContext(driver);

    ze_command_list_handle_t cmdListImmediate_1;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(context, device, &zeDefaultGPUImmediateCommandQueueDesc, &cmdListImmediate_1));
    ze_command_list_handle_t cmdListImmediate_2;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(context, device, &zeDefaultGPUImmediateCommandQueueDesc, &cmdListImmediate_2));
    ctx = {driver, context, device};
    return TestResult::Success;
}

TestResult create_kernel(L0CommonContext &ctx,
                         const std::string &kernelFileName,
                         const std::string &kernelName,
                         ze_kernel_handle_t &kernel,
                         ze_module_handle_t &module) {

    auto kernelBinary = FileHelper::loadBinaryFile(kernelFileName);
    if (kernelBinary.size() == 0) {
        return TestResult::KernelNotFound;
    }
    kernelBinary.push_back('\0'); // null-terminate for safety

    ze_module_desc_t moduleDesc{ZE_STRUCTURE_TYPE_MODULE_DESC};
    moduleDesc.format = ZE_MODULE_FORMAT_OCLC;
    moduleDesc.inputSize = kernelBinary.size();
    moduleDesc.pInputModule = kernelBinary.data();

    ASSERT_ZE_RESULT_SUCCESS(zeModuleCreate(ctx.context, ctx.device, &moduleDesc, &module, nullptr));
    ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
    kernelDesc.flags = ZE_KERNEL_FLAG_EXPLICIT_RESIDENCY;
    kernelDesc.pKernelName = kernelName.c_str();
    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &kernel));
    return TestResult::Success;
}
