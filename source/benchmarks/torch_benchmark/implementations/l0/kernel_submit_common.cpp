/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "kernel_submit_common.hpp"

L0Context::L0Context() {
    if (zeCommandListCreateImmediate(l0.context, l0.device, &zeDefaultGPUImmediateCommandQueueDesc, &cmdListImmediate_1) != ZE_RESULT_SUCCESS) {
        throw std::runtime_error("Failed to create immediate command list 1");
    }
    if (zeCommandListCreateImmediate(l0.context, l0.device, &zeDefaultGPUImmediateCommandQueueDesc, &cmdListImmediate_2) != ZE_RESULT_SUCCESS) {
        zeCommandListDestroy(cmdListImmediate_1);
        throw std::runtime_error("Failed to create immediate command list 2");
    }
}

L0Context::~L0Context() {
    if (cmdListImmediate_1) {
        zeCommandListDestroy(cmdListImmediate_1);
    }
    if (cmdListImmediate_2) {
        zeCommandListDestroy(cmdListImmediate_2);
    }
}

TestResult Kernel::create_kernel(LevelZero &l0,
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

Kernel::Kernel(LevelZero &l0, 
                const std::string &kernelFileName,
                const std::string &kernelName) {

    TestResult result = create_kernel(l0, kernelFileName, kernelName, kernel, module);
    
    if (result == TestResult::KernelNotFound) {
        throw std::runtime_error("Kernel file not found: " + kernelFileName);
    } else if (result != TestResult::Success) {
        throw std::runtime_error("Failed to create kernel");
    }
}

Kernel::~Kernel() {
    zeKernelDestroy(kernel);
    zeModuleDestroy(module);
}

ze_kernel_handle_t Kernel::get() const {
    return kernel;
}

BatchingLoop::BatchingLoop(ze_command_list_handle_t list, size_t batch) 
    : cmdList(list), batchSize(batch) {}

BatchingLoop::~BatchingLoop() {
    zeCommandListHostSynchronize(cmdList, UINT64_MAX);
}

void BatchingLoop::checkBatch(size_t i) {
    if (batchSize > 0 && ((i + 1) % batchSize) == 0) {
        zeCommandListHostSynchronize(cmdList, UINT64_MAX);
    }
}

// usunąć później
TestResult create_kernel(LevelZero &l0,
                         const std::string &kernelFileName,
                         const std::string &kernelName,
                         ze_kernel_handle_t &kernel,
                         ze_module_handle_t &module) {
    return Kernel::create_kernel(l0, kernelFileName, kernelName, kernel, module);
}
