/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/set_kernel_arg_immediate.h"

#include <gtest/gtest.h>

typedef struct {
    int32_t values[2];
} st_input_8;

typedef struct {
    int32_t values[16];
} st_input_64;

typedef struct {
    int32_t values[64];
} st_input_256;

typedef struct {
    int32_t values[128];
} st_input_512;

typedef struct {
    int32_t values[256];
} st_input_1024;

typedef struct {
    int32_t values[512];
} st_input_2048;

static TestResult run(const KernelSetArgumentValueImmediateArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    LevelZero levelzero;
    Timer timer;

    // Check max argument size
    ze_device_module_properties_t moduleProperties{};
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetModuleProperties(levelzero.device, &moduleProperties));
    if (arguments.argumentSize > moduleProperties.maxArgumentsSize) {
        return TestResult::DeviceNotCapable;
    }

    // Create kernel
    auto spirvModule = FileHelper::loadBinaryFile(std::string("api_overhead_benchmark_") + std::to_string(arguments.argumentSize) + "bytes_argument.spv");
    if (spirvModule.size() == 0) {
        return TestResult::KernelNotFound;
    }
    ze_module_handle_t module;
    ze_kernel_handle_t kernel;
    ze_module_desc_t moduleDesc{ZE_STRUCTURE_TYPE_MODULE_DESC};
    moduleDesc.format = ZE_MODULE_FORMAT_IL_SPIRV;
    moduleDesc.pInputModule = reinterpret_cast<const uint8_t *>(spirvModule.data());
    moduleDesc.inputSize = spirvModule.size();
    ASSERT_ZE_RESULT_SUCCESS(zeModuleCreate(levelzero.context, levelzero.device, &moduleDesc, &module, nullptr));
    ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
    kernelDesc.pKernelName = "arg_size";
    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &kernel));

    st_input_8 kernelArgument8{};
    st_input_64 kernelArgument64{};
    st_input_256 kernelArgument256{};
    st_input_512 kernelArgument512{};
    st_input_1024 kernelArgument1024{};
    st_input_2048 kernelArgument2048{};

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        if (arguments.differentValues) {
            ++kernelArgument8.values[1];
            ++kernelArgument64.values[15];
            ++kernelArgument256.values[63];
            ++kernelArgument512.values[127];
            ++kernelArgument1024.values[255];
            ++kernelArgument2048.values[511];
        }
        switch (arguments.argumentSize) {
        case 8:
            timer.measureStart();
            ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(st_input_8), &kernelArgument8));
            timer.measureEnd();
            break;
        case 64:
            timer.measureStart();
            ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(st_input_64), &kernelArgument64));
            timer.measureEnd();
            break;
        case 256:
            timer.measureStart();
            ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(st_input_256), &kernelArgument256));
            timer.measureEnd();
            break;
        case 512:
            timer.measureStart();
            ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(st_input_512), &kernelArgument512));
            timer.measureEnd();
            break;
        case 1024:
            timer.measureStart();
            ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(st_input_1024), &kernelArgument1024));
            timer.measureEnd();
            break;
        case 2048:
            timer.measureStart();
            ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(st_input_2048), &kernelArgument2048));
            timer.measureEnd();
            break;
        default:
            return TestResult::InvalidArgs;
        }
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<KernelSetArgumentValueImmediate> registerTestCase(run, Api::L0);
