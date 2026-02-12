/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "kernel_submit_common.hpp"

L0Context::L0Context() : L0Context(ExtensionProperties::create()) {}

L0Context::L0Context(const ExtensionProperties &extensionProperties, bool useInOrderQueue) : l0(extensionProperties) {
    ze_command_queue_desc_t queueDesc = zeDefaultGPUImmediateCommandQueueDesc;
    if (!useInOrderQueue) {
        queueDesc.flags = ZE_COMMAND_QUEUE_FLAG_COPY_OFFLOAD_HINT;
    }

    if (zeCommandListCreateImmediate(l0.context, l0.device, &queueDesc, &cmdListImmediate_1) != ZE_RESULT_SUCCESS) {
        throw std::runtime_error("Failed to create immediate command list 1");
    }
    if (zeCommandListCreateImmediate(l0.context, l0.device, &queueDesc, &cmdListImmediate_2) != ZE_RESULT_SUCCESS) {
        zeCommandListDestroy(cmdListImmediate_1);
        throw std::runtime_error("Failed to create immediate command list 2");
    }
    if (zeCommandListCreateImmediate(l0.context, l0.device, &queueDesc, &cmdListImmediate_3) != ZE_RESULT_SUCCESS) {
        zeCommandListDestroy(cmdListImmediate_1);
        zeCommandListDestroy(cmdListImmediate_2);
        throw std::runtime_error("Failed to create immediate command list 3");
    }
}

L0Context::~L0Context() {
    if (cmdListImmediate_1) {
        zeCommandListDestroy(cmdListImmediate_1);
    }
    if (cmdListImmediate_2) {
        zeCommandListDestroy(cmdListImmediate_2);
    }
    if (cmdListImmediate_3) {
        zeCommandListDestroy(cmdListImmediate_3);
    }
}

TestResult create_kernel(LevelZero &l0,
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

    ASSERT_ZE_RESULT_SUCCESS(zeModuleCreate(l0.context, l0.device, &moduleDesc, &module, nullptr));
    ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
    kernelDesc.flags = ZE_KERNEL_FLAG_EXPLICIT_RESIDENCY;
    kernelDesc.pKernelName = kernelName.c_str();
    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &kernel));
    return TestResult::Success;
}

TestResult create_counter_based_event(LevelZero &ctx, ze_event_handle_t &event, bool enableProfiling) {
    zex_counter_based_event_desc_t desc = defaultCounterBasedEventDesc;
    desc.flags |= enableProfiling ? ZEX_COUNTER_BASED_EVENT_FLAG_KERNEL_TIMESTAMP : 0;

    ASSERT_ZE_RESULT_SUCCESS(ctx.counterBasedEventCreate2(ctx.context, ctx.device, &desc, &event));
    return TestResult::Success;
}
