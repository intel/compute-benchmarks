/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"
#include "framework/ur/error.h"
#include "framework/ur/ur.h"
#include "framework/ur/usm_helper.h"
#include "framework/utility/timer.h"

#include "definitions/usm_memory_allocation.h"

#include <gtest/gtest.h>

static TestResult isAsyncSupported(ur_device_handle_t device, const UsmMemoryAllocationArguments &arguments) {
    // check if async USM allocations are supported
    ur_bool_t usmAllocsSupport = false;
    auto status = urDeviceGetInfo(device, UR_DEVICE_INFO_ASYNC_USM_ALLOCATIONS_SUPPORT_EXP,
                                  sizeof(usmAllocsSupport), &usmAllocsSupport, nullptr);

    if (status != UR_RESULT_SUCCESS || !usmAllocsSupport) {
        std::cerr << "Async USM allocations are not supported!" << std::endl;
        return TestResult::DeviceNotCapable;
    }

    // select the correct support flag for the requested placement type
    ur_device_info_t supportFlag;
    switch (arguments.usmMemoryPlacement) {
    case UsmRuntimeMemoryPlacement::Host:
        supportFlag = UR_DEVICE_INFO_USM_HOST_SUPPORT;
        break;
    case UsmRuntimeMemoryPlacement::Device:
        supportFlag = UR_DEVICE_INFO_USM_DEVICE_SUPPORT;
        break;
    case UsmRuntimeMemoryPlacement::Shared:
        supportFlag = UR_DEVICE_INFO_USM_SINGLE_SHARED_SUPPORT;
        break;
    default:
        return TestResult::InvalidArgs;
    }

    // check if async USM is supported on the requested placement type
    ur_device_usm_access_capability_flags_t usmSupport = false;
    auto status2 = urDeviceGetInfo(device, supportFlag,
                                   sizeof(usmSupport), &usmSupport, nullptr);
    if (status2 != UR_RESULT_SUCCESS || !(usmSupport & UR_DEVICE_USM_ACCESS_CAPABILITY_FLAG_ACCESS)) {
        std::cerr << "Async USM is not supported on this memory placement!" << std::endl;
        return TestResult::DeviceNotCapable;
    }

    return TestResult::Success;
}

static TestResult run(const UsmMemoryAllocationArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    UrState ur;
    ur_queue_handle_t queue = nullptr;
    ur_queue_properties_t queueProperties = {};
    Timer timer;

    void *ptr{};

    bool isAsync = false;
    switch (arguments.strategy) {
    case MemoryStrategy::Sync:
        isAsync = false;
        break;
    case MemoryStrategy::Async: {
        TestResult asyncSupportResult = isAsyncSupported(ur.device, arguments);
        if (asyncSupportResult != TestResult::Success) {
            return asyncSupportResult;
        }
        isAsync = true;
        ASSERT_UR_RESULT_SUCCESS(urQueueCreate(ur.context, ur.device, &queueProperties, &queue));
        break;
    }
    default:
        std::cerr << "use either Sync or Async memory strategy" << std::endl;
        return TestResult::InvalidArgs;
    }

    // Benchmark
    for (auto j = 0u; j < arguments.iterations; j++) {
        if (arguments.measureMode == AllocationMeasureMode::Allocate ||
            arguments.measureMode == AllocationMeasureMode::Both) {
            timer.measureStart();
        }

        if (isAsync) {
            ASSERT_UR_RESULT_SUCCESS(UR::UsmHelper::allocate_async(arguments.usmMemoryPlacement, queue, arguments.size, &ptr));
        } else {
            ASSERT_UR_RESULT_SUCCESS(UR::UsmHelper::allocate(arguments.usmMemoryPlacement, ur.context, ur.device, arguments.size, &ptr));
        }

        if (arguments.measureMode == AllocationMeasureMode::Allocate) {
            timer.measureEnd();
        } else if (arguments.measureMode == AllocationMeasureMode::Free) {
            timer.measureStart();
        }

        if (isAsync) {
            ASSERT_UR_RESULT_SUCCESS(urEnqueueUSMFreeExp(queue, nullptr, ptr, 0, nullptr, nullptr));
        } else {
            ASSERT_UR_RESULT_SUCCESS(urUSMFree(ur.context, ptr));
        }

        if (arguments.measureMode == AllocationMeasureMode::Free ||
            arguments.measureMode == AllocationMeasureMode::Both) {
            timer.measureEnd();
        }

        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
        if (isAsync) {
            ASSERT_UR_RESULT_SUCCESS(urQueueFinish(queue));
        }
    }
    if (isAsync) {
        EXPECT_UR_RESULT_SUCCESS(urQueueRelease(queue));
    }
    return TestResult::Success;
}

static RegisterTestCaseImplementation<UsmMemoryAllocation> registerTestCase(run, Api::UR);
