/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"
#include "framework/utility/windows/cpu_time_timer.h"

#include "definitions/event_host_synchronize.h"

#include <chrono>
#include <gtest/gtest.h>
#include <limits>

namespace {
double getCpuUtilizationPercent(std::chrono::nanoseconds cpuTime, Timer::Clock::duration wallTime) {
    const auto cpuSeconds = std::chrono::duration<double>(cpuTime).count();
    const auto wallSeconds = std::chrono::duration<double>(wallTime).count();
    if (wallSeconds == 0.0) {
        return 0.0;
    }
    return 100.0 * cpuSeconds / wallSeconds;
}
} // namespace

static TestResult run(const EventHostSynchronizeArguments &arguments, Statistics &statistics) {
    MeasurementFields latencySelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(latencySelector.getUnit(), latencySelector.getType());
        return TestResult::Nooped;
    }

    ExtensionProperties extensionProperties = ExtensionProperties::create();
    LevelZero levelzero(extensionProperties);
    const bool counterBasedEvents = arguments.inOrderQueue;
    if (counterBasedEvents && !levelzero.isCounterBasedEventsSupported()) {
        return TestResult::ApiNotCapable;
    }

    Timer wallTimer;
    CpuTimeTimer threadCpuTimer(CpuTimeTimer::Scope::Thread);
    CpuTimeTimer processCpuTimer(CpuTimeTimer::Scope::Process);

    auto spirvModule = FileHelper::loadBinaryFile("cpu_efficiency_benchmark_eat_time.spv");
    if (spirvModule.empty()) {
        return TestResult::KernelNotFound;
    }

    ze_module_handle_t module{};
    ze_module_desc_t moduleDesc{ZE_STRUCTURE_TYPE_MODULE_DESC};
    moduleDesc.format = ZE_MODULE_FORMAT_IL_SPIRV;
    moduleDesc.pInputModule = reinterpret_cast<const uint8_t *>(spirvModule.data());
    moduleDesc.inputSize = spirvModule.size();
    ASSERT_ZE_RESULT_SUCCESS(zeModuleCreate(levelzero.context, levelzero.device, &moduleDesc, &module, nullptr));

    ze_kernel_handle_t kernel{};
    ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
    kernelDesc.pKernelName = "eat_time";
    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &kernel));

    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, 1u, 1u, 1u));
    const int kernelOperationsCount = static_cast<int>(static_cast<size_t>(arguments.kernelExecutionTime) * 8u);
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(kernelOperationsCount), &kernelOperationsCount));

    ze_event_pool_handle_t eventPool{};
    ze_event_handle_t event{};
    if (counterBasedEvents) {
        ze_event_counter_based_flags_t eventFlags = ZE_EVENT_COUNTER_BASED_FLAG_IMMEDIATE | ZE_EVENT_COUNTER_BASED_FLAG_HOST_VISIBLE;
        if (arguments.useKernelTimestamps) {
            eventFlags |= ZE_EVENT_COUNTER_BASED_FLAG_DEVICE_TIMESTAMP;
        }

        ze_event_counter_based_desc_t eventDesc{.stype = ZE_STRUCTURE_TYPE_EVENT_COUNTER_BASED_DESC,
                                                .pNext = nullptr,
                                                .flags = eventFlags,
                                                .signal = ZE_EVENT_SCOPE_FLAG_HOST,
                                                .wait = ZE_EVENT_SCOPE_FLAG_HOST};
        ASSERT_ZE_RESULT_SUCCESS(zeEventCounterBasedCreate(levelzero.context, levelzero.device, &eventDesc, &event));
    } else {
        ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
        eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
        if (arguments.useKernelTimestamps) {
            eventPoolDesc.flags |= ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP;
        }
        eventPoolDesc.count = 1;
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.device, &eventPool));

        ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
        eventDesc.index = 0;
        eventDesc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
        eventDesc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
        ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &event));
    }

    ze_command_queue_desc_t commandQueueDesc = levelzero.commandQueueDesc;
    commandQueueDesc.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
    if (arguments.inOrderQueue) {
        commandQueueDesc.flags |= ZE_COMMAND_QUEUE_FLAG_IN_ORDER;
    }
    ze_command_list_handle_t cmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc, &cmdList));

    const ze_group_count_t groupCount{1u, 1u, 1u};
    constexpr auto timeout = std::numeric_limits<uint64_t>::max();
    const auto batchCount = static_cast<size_t>(arguments.batchSize);
    const auto batchSize = static_cast<Timer::Clock::duration::rep>(static_cast<size_t>(arguments.batchSize));

    for (auto i = 0u; i < arguments.iterations; i++) {
        Timer::Clock::duration totalWallTime{};
        std::chrono::nanoseconds totalThreadCpuTime{};
        std::chrono::nanoseconds totalProcessCpuTime{};

        for (size_t batchIndex = 0u; batchIndex < batchCount; batchIndex++) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, event, 0, nullptr));

            threadCpuTimer.measureStart();
            processCpuTimer.measureStart();
            wallTimer.measureStart();
            const auto synchronizeResult = zeEventHostSynchronize(event, timeout);
            wallTimer.measureEnd();
            processCpuTimer.measureEnd();
            threadCpuTimer.measureEnd();

            ASSERT_ZE_RESULT_SUCCESS(synchronizeResult);

            totalWallTime += wallTimer.get();
            totalThreadCpuTime += threadCpuTimer.get();
            totalProcessCpuTime += processCpuTimer.get();

            if (!counterBasedEvents) {
                ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(event));
            }
        }

        statistics.pushValue(totalWallTime / batchSize, latencySelector.getUnit(), latencySelector.getType(), "latency");
        statistics.pushValue(std::chrono::duration_cast<Statistics::Clock::duration>(totalThreadCpuTime / batchSize), MeasurementUnit::Microseconds, MeasurementType::Cpu, "threadCpuTime");
        statistics.pushPercentage(getCpuUtilizationPercent(totalThreadCpuTime, totalWallTime), MeasurementUnit::Percentage, MeasurementType::Cpu, "threadCpuUtilization");
        statistics.pushValue(std::chrono::duration_cast<Statistics::Clock::duration>(totalProcessCpuTime / batchSize), MeasurementUnit::Microseconds, MeasurementType::Cpu, "processCpuTime");
        statistics.pushPercentage(getCpuUtilizationPercent(totalProcessCpuTime, totalWallTime), MeasurementUnit::Percentage, MeasurementType::Cpu, "processCpuUtilization");
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
    if (eventPool != nullptr) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<EventHostSynchronize> registerTestCase(run, Api::L0);
