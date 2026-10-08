/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/set_kernel_arg_immediate.h"

#include <gtest/gtest.h>

struct KernelArgument8 {
    int32_t values[2];
};

struct KernelArgument64 {
    int32_t values[16];
};

struct KernelArgument256 {
    int32_t values[64];
};

struct KernelArgument512 {
    int32_t values[128];
};

struct KernelArgument1024 {
    int32_t values[256];
};

struct KernelArgument2048 {
    int32_t values[512];
};

static TestResult run(const KernelSetArgumentValueImmediateArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    LevelZero levelzero;
    Timer timer;

    ze_device_module_properties_t moduleProperties{};
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetModuleProperties(levelzero.device, &moduleProperties));
    if (arguments.argumentSize > moduleProperties.maxArgumentsSize) {
        return TestResult::DeviceNotCapable;
    }

    ze_module_handle_t module;
    ze_kernel_handle_t kernel;
    if (auto result = L0::KernelHelper::loadKernel(levelzero, std::string("api_overhead_benchmark_") + std::to_string(arguments.argumentSize) + "bytes_argument.cl", "arg_size", &kernel, &module, nullptr);
        result != TestResult::Success) {
        return result;
    }

    KernelArgument8 kernelArgument8{};
    KernelArgument64 kernelArgument64{};
    KernelArgument256 kernelArgument256{};
    KernelArgument512 kernelArgument512{};
    KernelArgument1024 kernelArgument1024{};
    KernelArgument2048 kernelArgument2048{};

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
            ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(KernelArgument8), &kernelArgument8));
            timer.measureEnd();
            break;
        case 64:
            timer.measureStart();
            ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(KernelArgument64), &kernelArgument64));
            timer.measureEnd();
            break;
        case 256:
            timer.measureStart();
            ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(KernelArgument256), &kernelArgument256));
            timer.measureEnd();
            break;
        case 512:
            timer.measureStart();
            ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(KernelArgument512), &kernelArgument512));
            timer.measureEnd();
            break;
        case 1024:
            timer.measureStart();
            ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(KernelArgument1024), &kernelArgument1024));
            timer.measureEnd();
            break;
        case 2048:
            timer.measureStart();
            ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(KernelArgument2048), &kernelArgument2048));
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
