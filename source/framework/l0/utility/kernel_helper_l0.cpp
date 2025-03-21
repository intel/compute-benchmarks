/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "kernel_helper_l0.h"

#include "framework/utility/file_helper.h"

namespace L0::KernelHelper {
TestResult loadKernel(LevelZero &levelzero, std::string spirv_file_name, std::string kernel_name, ze_kernel_handle_t *kernel_out,
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

    ASSERT_ZE_RESULT_SUCCESS(zeModuleCreate(levelzero.context, levelzero.device,
                                            &moduleDesc, module_out, nullptr));

    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(*module_out, &kernelDesc, kernel_out));
    return TestResult::Success;
}
} // namespace L0::KernelHelper
