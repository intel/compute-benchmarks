/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "kernel_helper_l0.h"

#include "framework/utility/file_helper.h"

namespace L0::KernelHelper {
TestResult loadModule(LevelZero &levelzero, const std::string &filePath, ze_module_handle_t *module, const char *pBuildFlags) {
    return loadModule(levelzero, levelzero.device, filePath, module, pBuildFlags);
}

TestResult loadModule(LevelZero &levelzero, ze_device_handle_t device, const std::string &filePath, ze_module_handle_t *module, const char *pBuildFlags) {
    auto sourceFile = FileHelper::loadTextFile(filePath);
    if (sourceFile.size() == 0) {
        return TestResult::KernelNotFound;
    }
    // OCLC input is a C string: without this a kernel file that does not end
    // with a newline loses its last character
    if (sourceFile.back() != '\0') {
        sourceFile.push_back('\0');
    }

    ze_module_desc_t moduleDesc{ZE_STRUCTURE_TYPE_MODULE_DESC};
    moduleDesc.format = ZE_MODULE_FORMAT_OCLC;
    moduleDesc.pInputModule = reinterpret_cast<const uint8_t *>(sourceFile.data());
    moduleDesc.inputSize = sourceFile.size();
    moduleDesc.pBuildFlags = pBuildFlags;
    ze_module_build_log_handle_t buildLog = nullptr;
    auto status = zeModuleCreate(levelzero.context, device, &moduleDesc, module, &buildLog);

    if (status != ZE_RESULT_SUCCESS) {
        std::string errorMessage = "zeModuleCreate failed with error code " + std::string(l0ErrorToString(status));
        size_t logSize = 0;
        if (buildLog != nullptr && zeModuleBuildLogGetString(buildLog, &logSize, nullptr) == ZE_RESULT_SUCCESS) {
            std::vector<char> buildLogString(logSize);
            if (zeModuleBuildLogGetString(buildLog, &logSize, buildLogString.data()) == ZE_RESULT_SUCCESS) {
                errorMessage += ". Build log:\n" + std::string(buildLogString.data());
            }
        }
        std::cout << errorMessage << std::endl;
    }

    if (buildLog != nullptr) {
        ASSERT_ZE_RESULT_SUCCESS(zeModuleBuildLogDestroy(buildLog));
    }
    return status == ZE_RESULT_SUCCESS ? TestResult::Success : TestResult::KernelBuildError;
}

TestResult loadKernel(LevelZero &levelzero, const std::string &filePath, const std::string &kernelName, ze_kernel_handle_t *kernel,
                      ze_module_handle_t *module, const char *pBuildFlags) {
    return loadKernel(levelzero, filePath, kernelName, kernel, module, pBuildFlags, 0);
}

TestResult loadKernel(LevelZero &levelzero, const std::string &filePath, const std::string &kernelName, ze_kernel_handle_t *kernel,
                      ze_module_handle_t *module, const char *pBuildFlags, ze_kernel_flags_t kernelFlags) {
    return loadKernel(levelzero, levelzero.device, filePath, kernelName, kernel, module, pBuildFlags, kernelFlags);
}

TestResult loadKernel(LevelZero &levelzero, ze_device_handle_t device, const std::string &filePath, const std::string &kernelName, ze_kernel_handle_t *kernel,
                      ze_module_handle_t *module, const char *pBuildFlags, ze_kernel_flags_t kernelFlags) {
    if (auto result = loadModule(levelzero, device, filePath, module, pBuildFlags); result != TestResult::Success) {
        return result;
    }

    ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
    kernelDesc.flags = kernelFlags;
    kernelDesc.pKernelName = kernelName.c_str();
    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(*module, &kernelDesc, kernel));
    return TestResult::Success;
}
} // namespace L0::KernelHelper
