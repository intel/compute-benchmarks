/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "kernel_helper_l0.h"

#include "framework/utility/file_helper.h"

namespace L0::KernelHelper {
TestResult loadKernel(LevelZero &levelzero, const std::string &filePath, const std::string &kernelName, ze_kernel_handle_t *kernel,
                      ze_module_handle_t *module) {
    auto sourceFile = FileHelper::loadTextFile(filePath);
    if (sourceFile.size() == 0) {
        return TestResult::KernelNotFound;
    }

    ze_module_desc_t moduleDesc{ZE_STRUCTURE_TYPE_MODULE_DESC};
    moduleDesc.format = ZE_MODULE_FORMAT_OCLC;
    moduleDesc.pInputModule = reinterpret_cast<const uint8_t *>(sourceFile.data());
    moduleDesc.inputSize = sourceFile.size();
    ASSERT_ZE_RESULT_SUCCESS(zeModuleCreate(levelzero.context, levelzero.device,
                                            &moduleDesc, module, nullptr));

    ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
    kernelDesc.pKernelName = kernelName.c_str();
    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(*module, &kernelDesc, kernel));
    return TestResult::Success;
}
} // namespace L0::KernelHelper
