/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/bit_operations_helper.h"
#include "framework/utility/timer.h"

#include "definitions/usm_eu_reduce_copy_ring.h"

#include <algorithm>
#include <gtest/gtest.h>
#include <level_zero/zer_api.h>
#include <vector>

static TestResult run(const UsmEUReduceCopyRingArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::GigabytesPerSecond, arguments.useEvents ? MeasurementType::Gpu : MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    constexpr size_t elementSize = 4 * sizeof(float);
    constexpr uint32_t workGroupSize = 64u;
    const bool throttled = arguments.throttledWorkItems > 0;
    if (arguments.numDevices < 2 || arguments.size % (elementSize * workGroupSize) != 0 ||
        arguments.throttledWorkItems < 0 || arguments.throttledWorkItems % workGroupSize != 0) {
        return TestResult::InvalidArgs;
    }
    const uint64_t workItems = throttled ? static_cast<uint64_t>(arguments.throttledWorkItems) : arguments.size / elementSize;
    if (workItems / workGroupSize > std::numeric_limits<uint32_t>::max()) {
        return TestResult::InvalidArgs;
    }

    LevelZero levelzero;
    const size_t numDevices = static_cast<size_t>(arguments.numDevices);
    if (levelzero.rootDevices.size() < numDevices) {
        return TestResult::DeviceNotCapable;
    }
    const std::vector<ze_device_handle_t> devices(levelzero.rootDevices.begin(), levelzero.rootDevices.begin() + numDevices);

    for (size_t i = 0; i < numDevices; i++) {
        ze_bool_t hasAccess = false;
        ASSERT_ZE_RESULT_SUCCESS(zeDeviceCanAccessPeer(devices[i], devices[(i + 1) % numDevices], &hasAccess));
        if (!hasAccess) {
            return TestResult::DeviceNotCapable;
        }
    }

    struct PerDevice {
        ze_command_list_handle_t cmdList = nullptr;
        ze_module_handle_t module = nullptr;
        ze_kernel_handle_t kernel = nullptr;
        void *in1 = nullptr;
        void *in2 = nullptr;
        void *out1 = nullptr;
        void *recv = nullptr;
        uint64_t timerResolution = 0;
        uint32_t timestampValidBits = 0;
    };
    std::vector<PerDevice> perDevice(numDevices);

    const char *kernelFile = throttled ? "p2p_benchmark_reduce_copy_throttled.cl" : "p2p_benchmark_reduce_copy.cl";
    const char *kernelName = throttled ? "reduce_copy_throttled" : "reduce_copy";
    for (size_t i = 0; i < numDevices; i++) {
        auto &d = perDevice[i];
        if (auto result = L0::KernelHelper::loadModule(levelzero, devices[i], kernelFile, &d.module, nullptr); result != TestResult::Success) {
            for (size_t j = 0; j < i; j++) {
                ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(perDevice[j].kernel));
                ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(perDevice[j].module));
            }
            return result;
        }
        ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
        kernelDesc.pKernelName = kernelName;
        ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(d.module, &kernelDesc, &d.kernel));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(d.kernel, workGroupSize, 1u, 1u));
    }

    const float initialValue = 1.0f;
    for (size_t i = 0; i < numDevices; i++) {
        auto &d = perDevice[i];
        ze_command_queue_desc_t commandQueueDesc = zeDefaultGPUImmediateCommandQueueDesc;
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, devices[i], &commandQueueDesc, &d.cmdList));
        d.timerResolution = levelzero.getTimerResolution(devices[i]);
        d.timestampValidBits = levelzero.getKernelTimestampValidBits(devices[i]);

        for (void **buffer : {&d.in1, &d.in2, &d.out1, &d.recv}) {
            ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmRuntimeMemoryPlacement::Device, levelzero, devices[i], arguments.size, buffer));
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryFill(d.cmdList, *buffer, &initialValue, sizeof(initialValue), arguments.size, nullptr, 0, nullptr));
        }
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(d.cmdList, std::numeric_limits<uint64_t>::max()));
    }

    for (size_t i = 0; i < numDevices; i++) {
        auto &d = perDevice[i];
        void *out2 = perDevice[(i + 1) % numDevices].recv;
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(d.kernel, 0, sizeof(d.in1), &d.in1));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(d.kernel, 1, sizeof(d.in2), &d.in2));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(d.kernel, 2, sizeof(d.out1), &d.out1));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(d.kernel, 3, sizeof(out2), &out2));
        if (throttled) {
            const uint64_t elementCount = arguments.size / elementSize;
            ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(d.kernel, 4, sizeof(elementCount), &elementCount));
        }
    }

    ze_event_pool_desc_t startEventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
    startEventPoolDesc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
    startEventPoolDesc.count = 1;
    ze_event_pool_handle_t startEventPool{};
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &startEventPoolDesc, static_cast<uint32_t>(numDevices), const_cast<ze_device_handle_t *>(devices.data()), &startEventPool));
    ze_event_desc_t startEventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
    startEventDesc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
    startEventDesc.wait = ZE_EVENT_SCOPE_FLAG_DEVICE;
    ze_event_handle_t startEvent{};
    ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(startEventPool, &startEventDesc, &startEvent));

    ze_event_pool_handle_t timestampEventPool{};
    std::vector<ze_event_handle_t> timestampEvents;
    if (arguments.useEvents) {
        ze_event_pool_desc_t timestampEventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
        timestampEventPoolDesc.flags = ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP | ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
        timestampEventPoolDesc.count = static_cast<uint32_t>(numDevices);
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &timestampEventPoolDesc, static_cast<uint32_t>(numDevices), const_cast<ze_device_handle_t *>(devices.data()), &timestampEventPool));
        timestampEvents.resize(numDevices);
        for (size_t i = 0; i < numDevices; i++) {
            ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
            eventDesc.index = static_cast<uint32_t>(i);
            eventDesc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
            eventDesc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
            ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(timestampEventPool, &eventDesc, &timestampEvents[i]));
        }
    }

    const ze_group_count_t groupCount{static_cast<uint32_t>(workItems / workGroupSize), 1u, 1u};
    const size_t totalPeerBytes = numDevices * arguments.size;
    Timer timer;
    for (auto iteration = 0u; iteration < arguments.iterations; iteration++) {
        for (size_t i = 0; i < numDevices; i++) {
            ze_event_handle_t signalEvent = arguments.useEvents ? timestampEvents[i] : nullptr;
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(perDevice[i].cmdList, perDevice[i].kernel, &groupCount, signalEvent, 1, &startEvent));
        }

        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSignal(startEvent));
        for (size_t i = 0; i < numDevices; i++) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(perDevice[i].cmdList, std::numeric_limits<uint64_t>::max()));
        }
        timer.measureEnd();

        if (arguments.useEvents) {
            std::chrono::nanoseconds longestKernelTime{0};
            for (size_t i = 0; i < numDevices; i++) {
                ze_kernel_timestamp_result_t timestampResult{};
                ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(timestampEvents[i], &timestampResult));
                const uint64_t ticks = BitHelper::isolateLowerNBits(timestampResult.global.kernelEnd - timestampResult.global.kernelStart, perDevice[i].timestampValidBits);
                const auto kernelTime = std::chrono::nanoseconds(ticks * perDevice[i].timerResolution);
                longestKernelTime = std::max(longestKernelTime, kernelTime);
            }
            statistics.pushValue(longestKernelTime, totalPeerBytes, typeSelector.getUnit(), typeSelector.getType());
        } else {
            statistics.pushValue(timer.get(), totalPeerBytes, typeSelector.getUnit(), typeSelector.getType());
        }

        ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(startEvent));
        for (auto event : timestampEvents) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(event));
        }
    }

    for (auto event : timestampEvents) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
    }
    if (timestampEventPool != nullptr) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(timestampEventPool));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(startEvent));
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(startEventPool));
    for (auto &d : perDevice) {
        ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(d.kernel));
        ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(d.module));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(d.cmdList));
        for (void *buffer : {d.in1, d.in2, d.out1, d.recv}) {
            ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, buffer));
        }
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<UsmEUReduceCopyRing> registerTestCase(run, Api::L0);
