/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/kernel_submit_slm_size.h"
#include "kernel_submit_common.hpp"

using data_type = float;

using L0DriverGetDefaultContext = decltype(&zeDriverGetDefaultContext);
using L0AppendLaunchKernelWithArguments = decltype(&zeCommandListAppendLaunchKernelWithArguments);

struct L0Context {
    L0CommonContext commonCtx;
    ze_command_list_handle_t cmdListImmediate;
};

static TestResult init_level_zero(L0Context &ctx) {
    ASSERT_TEST_RESULT_SUCCESS(init_level_zero_common(ctx.commonCtx));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(ctx.commonCtx.context, ctx.commonCtx.device, &zeDefaultGPUImmediateCommandQueueDesc, &ctx.cmdListImmediate));
    return TestResult::Success;
}

static TestResult compute_slm_num(int &slm_num, L0Context &ctx) {
    ze_device_compute_properties_t computeProps = {};
    computeProps.stype = ZE_STRUCTURE_TYPE_DEVICE_COMPUTE_PROPERTIES;
    zeDeviceGetComputeProperties(ctx.commonCtx.device, &computeProps);
    const uint32_t max_slm_size = computeProps.maxSharedLocalMemory;

    if (slm_num < 0) {
        slm_num = max_slm_size / sizeof(data_type);
    }

    if (static_cast<uint32_t>(slm_num * sizeof(data_type)) > max_slm_size) {
        return TestResult::Error;
    }
    return TestResult::Success;
}

static TestResult run_kernel(data_type *out_buf, int slm_num, L0Context &l0, ze_kernel_handle_t kernel) {
    ze_group_count_t dispatch{1, 1, 1}; // juju: was 1 instead of 512 for testing

    const int wgs = std::min(slm_num, 1024);
    int slm_size = slm_num * sizeof(data_type);
    void *kernelArguments[3] = {&out_buf, &slm_num, &slm_size};

    ze_group_size_t groupSizes = {static_cast<uint32_t>(wgs), 1, 1};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(l0.cmdListImmediate, kernel, dispatch, groupSizes,
                                                                          kernelArguments, nullptr, nullptr, 0, nullptr));
    return TestResult::Success;
}

static TestResult run(const KernelSubmitSlmSizeArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        std::cerr << "Noop run for Torch L0 benchmark" << std::endl;
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    L0Context l0{};
    ASSERT_TEST_RESULT_SUCCESS(init_level_zero(l0));

    ze_kernel_handle_t kernel{};
    ze_module_handle_t module{};
    ASSERT_TEST_RESULT_SUCCESS(create_kernel(l0.commonCtx, "torch_benchmark_elementwise_slm.cl", "torch_benchmark_elementwise_slm", kernel, module));

    int slm_num = static_cast<int>(arguments.slmNum);
    ASSERT_TEST_RESULT_SUCCESS(compute_slm_num(slm_num, l0));

    data_type *out_buf = l0_malloc_device<data_type>(l0.commonCtx, 2);
    if (!out_buf) {
        return TestResult::Error;
    }

    // warmup
    for (int i = 0; i < arguments.warmupIterations; i++) {
        ASSERT_TEST_RESULT_SUCCESS(run_kernel(out_buf, slm_num, l0, kernel));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(l0.cmdListImmediate, UINT64_MAX));

    for (size_t i = 0; i < arguments.iterations; ++i) {
        Timer timer;
        timer.measureStart();
        ASSERT_TEST_RESULT_SUCCESS(run_kernel(out_buf, slm_num, l0, kernel));
        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());

        // expect a wait here after a batch of submissions
        if (i > 0 && i % arguments.batchSize == 0) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(l0.cmdListImmediate, UINT64_MAX));
        }
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(l0.cmdListImmediate, UINT64_MAX));

    // clean up
    data_type host_result[2];
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(l0.cmdListImmediate, &host_result, out_buf, 2 * sizeof(data_type), nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(l0.cmdListImmediate, UINT64_MAX));

    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(l0.commonCtx.context, out_buf));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(l0.cmdListImmediate));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));

    if (host_result[1] > 13.1 || host_result[1] < 12.9) {
        std::cerr << "Wrong checker value: " << host_result[1] << ", expected 13.0, result value:" << host_result[0] << std::endl;
        return TestResult::Error;
    }

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<KernelSubmitSlmSize> registerTestCase(run, Api::L0, true);
