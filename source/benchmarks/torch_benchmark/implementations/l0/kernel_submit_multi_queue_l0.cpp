/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"

#include "../../../../../third_party/level-zero-sdk/include/level_zero/ze_api.h"
#include "../../../../framework/l0/utility/error.h"
#include "../../../../framework/test_case/test_result.h"
#include "definitions/kernel_submit_multi_queue.h"
#include "gtest/gtest.h"
#include "level_zero/zer_api.h"

#include <level_zero/ze_api.h>

using data_type = int;

struct L0Context {
    ze_driver_handle_t driver;
    ze_context_handle_t context;
    ze_device_handle_t device;
    ze_command_list_handle_t cmdListImmediate_1;
    ze_command_list_handle_t cmdListImmediate_2;
};

TestResult init_level_zero(L0Context &ctx) {
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
    ctx = {driver, context, device, cmdListImmediate_1, cmdListImmediate_2};
    return TestResult::Success;
}

inline data_type *l0_malloc_device(ze_context_handle_t context,
                                   ze_device_handle_t device,
                                   size_t length) {
    data_type *ptr = nullptr;
    if (zeMemAllocDevice(context, &zeDefaultGPUDeviceMemAllocDesc, length * sizeof(data_type), alignof(data_type), device, (void **)&ptr) != ZE_RESULT_SUCCESS) {
        return nullptr;
    }
    return ptr;
}

inline TestResult launch_kernel_l0(ze_command_list_handle_t cmdListImmediate,
                                   ze_kernel_handle_t kernel,
                                   ze_group_count_t dispatch,
                                   const uint32_t wgs,
                                   data_type *res,
                                   data_type *data_1,
                                   data_type *data_2,
                                   ze_event_handle_t signal_event = nullptr,
                                   ze_event_handle_t wait_event = nullptr) {
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, wgs, 1u, 1u));
    void *kernelArguments[3] = {res, data_1, data_2};
    ze_group_size_t groupSizes = {wgs, 1u, 1u};
    if (wait_event) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(cmdListImmediate, kernel, dispatch, groupSizes,
                                                                              kernelArguments, nullptr, nullptr, 1, &wait_event));
    } else {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(cmdListImmediate, kernel, dispatch, groupSizes,
                                                                              kernelArguments, nullptr, signal_event, 0, nullptr));
    }
    return TestResult::Success;
}

static TestResult run(const KernelSubmitMultiQueueArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        std::cerr << "Noop run for Torch L0 benchmark" << std::endl;
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    L0Context l0;
    ASSERT_TEST_RESULT_SUCCESS(init_level_zero(l0));
    const size_t length = static_cast<size_t>(arguments.workgroupCount * arguments.workgroupSize);

    const auto kernelBinary = FileHelper::loadBinaryFile("torch_benchmark_elementwise_sum_2.spv");
    if (kernelBinary.size() == 0) {
        return TestResult::KernelNotFound;
    }

    // create kernel
    ze_module_desc_t moduleDesc{ZE_STRUCTURE_TYPE_MODULE_DESC};
    moduleDesc.format = ZE_MODULE_FORMAT_IL_SPIRV;
    moduleDesc.inputSize = kernelBinary.size();
    moduleDesc.pInputModule = kernelBinary.data();
    ze_module_handle_t module{};
    ASSERT_ZE_RESULT_SUCCESS(zeModuleCreate(l0.context, l0.device, &moduleDesc, &module, nullptr));
    ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
    kernelDesc.flags = ZE_KERNEL_FLAG_EXPLICIT_RESIDENCY;
    kernelDesc.pKernelName = "elementwise_sum_2";
    ze_kernel_handle_t kernel{};
    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &kernel));

    data_type *d_a[2] = {};
    data_type *d_b[2] = {};
    data_type *d_c[2] = {};
    // allocate device memory
    for (int i = 0; i < 2; i++) {
        d_a[i] = l0_malloc_device(l0.context, l0.device, length);
        d_b[i] = l0_malloc_device(l0.context, l0.device, length);
        d_c[i] = l0_malloc_device(l0.context, l0.device, length);

        if (!d_a[i] || !d_b[i] || !d_c[i]) {
            return TestResult::Error;
        }
    }

    ze_group_count_t dispatch{static_cast<uint32_t>(arguments.workgroupCount), 1, 1};

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(l0.cmdListImmediate_1, UINT64_MAX));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(l0.cmdListImmediate_2, UINT64_MAX));

    // create counter-based event to sync cmdlist_1 and cmdlist_2
    ze_event_handle_t q2_last_event = nullptr;
    zex_counter_based_event_desc_t desc = {};
    desc.stype = ZEX_STRUCTURE_COUNTER_BASED_EVENT_DESC;
    desc.pNext = nullptr;
    desc.flags = ZEX_COUNTER_BASED_EVENT_FLAG_IMMEDIATE;

    L0CounterBasedEventCreate2 counterBasedEventCreate2 = nullptr;
    ASSERT_ZE_RESULT_SUCCESS(zeDriverGetExtensionFunctionAddress(
        l0.driver,
        "zexCounterBasedEventCreate2",
        reinterpret_cast<void **>(&counterBasedEventCreate2)));
    if (!counterBasedEventCreate2) {
        throw std::runtime_error("Driver does not support Counter-Based Event");
    }
    counterBasedEventCreate2(l0.context, l0.device, &desc, &q2_last_event);

    for (size_t i = 0; i < arguments.iterations; i++) {
        // submit several kernels into cmdlist_1
        for (int j = 0; j < arguments.kernelsPerQueue; j++) {

            ASSERT_TEST_RESULT_SUCCESS(launch_kernel_l0(l0.cmdListImmediate_1, kernel, dispatch, static_cast<uint32_t>(arguments.workgroupSize), d_a[0], d_b[0], d_c[0]));
        }
        // submit several kernels into cmdlist_2
        for (int j = 0; j < arguments.kernelsPerQueue - 1; j++) {
            ASSERT_TEST_RESULT_SUCCESS(launch_kernel_l0(l0.cmdListImmediate_2, kernel, dispatch, static_cast<uint32_t>(arguments.workgroupSize), d_a[1], d_b[1], d_c[1]));
        }
        ASSERT_TEST_RESULT_SUCCESS(launch_kernel_l0(l0.cmdListImmediate_2, kernel, dispatch, static_cast<uint32_t>(arguments.workgroupSize), d_a[1], d_b[1], d_c[1], q2_last_event, nullptr));
        // mark the last kernel in cmdlist_2
        Timer timer;
        timer.measureStart();
        ASSERT_TEST_RESULT_SUCCESS(launch_kernel_l0(l0.cmdListImmediate_1, kernel, dispatch, static_cast<uint32_t>(arguments.workgroupSize), d_a[0], d_b[0], d_c[0], nullptr, q2_last_event));
        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(l0.cmdListImmediate_1, UINT64_MAX));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(l0.cmdListImmediate_2, UINT64_MAX));

    // clean
    for (int i = 0; i < 2; i++) {
        if (d_a[i])
            ASSERT_ZE_RESULT_SUCCESS(zeMemFree(l0.context, d_a[i]));
        if (d_b[i])
            ASSERT_ZE_RESULT_SUCCESS(zeMemFree(l0.context, d_b[i]));
        if (d_c[i])
            ASSERT_ZE_RESULT_SUCCESS(zeMemFree(l0.context, d_c[i]));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(l0.cmdListImmediate_1));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(l0.cmdListImmediate_2));

    return TestResult::Success;
};

[[maybe_unused]] static RegisterTestCaseImplementation<KernelSubmitMultiQueue> registerTestCase(run, Api::L0, true);
