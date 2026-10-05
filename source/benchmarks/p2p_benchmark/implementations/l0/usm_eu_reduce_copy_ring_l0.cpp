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
#include <array>
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
    constexpr uint32_t barrierWorkGroupSize = 16u;
    const bool throttled = arguments.throttledWorkItems > 0;
    const size_t numDevices = static_cast<size_t>(arguments.numDevices);
    if (arguments.numDevices < 2 || arguments.throttledWorkItems < 0 || arguments.throttledWorkItems % workGroupSize != 0) {
        return TestResult::InvalidArgs;
    }
    if (throttled && arguments.size % (elementSize * workGroupSize) != 0) {
        return TestResult::InvalidArgs;
    }
    if (!throttled && (arguments.size % elementSize != 0 || arguments.size / elementSize % numDevices != 0)) {
        return TestResult::InvalidArgs;
    }

    const size_t pipelineSize = std::max<size_t>(numDevices, 3);
    const size_t chunkSize = arguments.tmpBufferSize / pipelineSize / elementSize * elementSize;
    const size_t partSize = arguments.size / numDevices;
    if (!throttled && chunkSize == 0) {
        return TestResult::InvalidArgs;
    }
    const uint64_t maxWorkItems = throttled ? static_cast<uint64_t>(arguments.throttledWorkItems) : partSize / elementSize;
    if ((maxWorkItems + workGroupSize - 1) / workGroupSize > std::numeric_limits<uint32_t>::max()) {
        return TestResult::InvalidArgs;
    }

    LevelZero levelzero;
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

    enum KernelType { ReduceCopy,
                      Copy,
                      Barrier,
                      KernelTypeCount };

    struct Launch {
        KernelType type;
        std::array<void *, 4> pointers;
        size_t numPointers;
        uint64_t count;
    };

    struct PerDevice {
        ze_command_list_handle_t cmdList = nullptr;
        ze_module_handle_t module = nullptr;
        std::array<ze_kernel_handle_t, KernelTypeCount> kernels{};
        std::vector<void *> buffers;
        std::vector<Launch> launches;
        uint64_t timerResolution = 0;
        uint32_t timestampValidBits = 0;
    };
    std::vector<PerDevice> perDevice(numDevices);

    const char *kernelFile = throttled ? "p2p_benchmark_reduce_copy_throttled.cl" : "p2p_benchmark_reduce_copy.cl";
    const char *buildFlags = throttled ? nullptr : "-cl-std=CL3.0";
    const std::array<const char *, KernelTypeCount> kernelNames = {throttled ? "reduce_copy_throttled" : "reduce_copy", "copy", "p2p_barrier"};
    const size_t kernelCount = throttled ? 1 : KernelTypeCount;
    for (size_t i = 0; i < numDevices; i++) {
        auto &d = perDevice[i];
        if (auto result = L0::KernelHelper::loadModule(levelzero, devices[i], kernelFile, &d.module, buildFlags); result != TestResult::Success) {
            for (size_t j = 0; j < i; j++) {
                for (size_t k = 0; k < kernelCount; k++) {
                    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(perDevice[j].kernels[k]));
                }
                ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(perDevice[j].module));
            }
            return result;
        }
        for (size_t k = 0; k < kernelCount; k++) {
            ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
            kernelDesc.pKernelName = kernelNames[k];
            ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(d.module, &kernelDesc, &d.kernels[k]));
            ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(d.kernels[k], k == static_cast<size_t>(Barrier) ? barrierWorkGroupSize : workGroupSize, 1u, 1u));
        }
    }

    const float initialValue = 1.0f;
    const uint32_t zero = 0u;
    const size_t syncSize = 2 * sizeof(uint32_t);
    const std::vector<size_t> bufferSizes = throttled ? std::vector<size_t>{arguments.size, arguments.size, arguments.size, arguments.size}
                                                      : std::vector<size_t>{arguments.size, arguments.size, pipelineSize * chunkSize, syncSize};
    for (size_t i = 0; i < numDevices; i++) {
        auto &d = perDevice[i];
        ze_command_queue_desc_t commandQueueDesc = zeDefaultGPUImmediateCommandQueueDesc;
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, devices[i], &commandQueueDesc, &d.cmdList));
        d.timerResolution = levelzero.getTimerResolution(devices[i]);
        d.timestampValidBits = levelzero.getKernelTimestampValidBits(devices[i]);

        d.buffers.resize(bufferSizes.size());
        for (size_t b = 0; b < bufferSizes.size(); b++) {
            const bool isSync = !throttled && b == 3;
            if (isSync) {
                ze_device_mem_alloc_desc_t syncAllocDesc{ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC};
                syncAllocDesc.flags = ZE_DEVICE_MEM_ALLOC_FLAG_BIAS_UNCACHED;
                ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &syncAllocDesc, bufferSizes[b], 0, devices[i], &d.buffers[b]));
            } else {
                ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmRuntimeMemoryPlacement::Device, levelzero, devices[i], bufferSizes[b], &d.buffers[b]));
            }
            const void *pattern = isSync ? static_cast<const void *>(&zero) : static_cast<const void *>(&initialValue);
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryFill(d.cmdList, d.buffers[b], pattern, isSync ? sizeof(zero) : sizeof(initialValue), bufferSizes[b], nullptr, 0, nullptr));
        }
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(d.cmdList, std::numeric_limits<uint64_t>::max()));
    }

    for (size_t i = 0; i < numDevices; i++) {
        auto &d = perDevice[i];
        auto &next = perDevice[(i + 1) % numDevices];
        if (throttled) {
            d.launches.push_back({ReduceCopy, {d.buffers[0], d.buffers[1], d.buffers[2], next.buffers[3]}, 4, arguments.size / elementSize});
            continue;
        }

        char *send = static_cast<char *>(d.buffers[0]);
        char *recv = static_cast<char *>(d.buffers[1]);
        char *tmp = static_cast<char *>(d.buffers[2]);
        char *nextRecv = static_cast<char *>(next.buffers[1]);
        char *nextTmp = static_cast<char *>(next.buffers[2]);
        const Launch barrier{Barrier, {d.buffers[3], next.buffers[3]}, 2, 0};
        const size_t rank = i;
        const size_t numChunks = partSize / chunkSize + (partSize % chunkSize != 0);

        d.launches.push_back(barrier);
        size_t slot = 0;
        for (size_t chunk = 0; chunk < numChunks; chunk++) {
            const size_t chunkOffset = chunk * chunkSize;
            const size_t dataSize = std::min(chunkSize, partSize - chunkOffset);
            size_t s = (rank + numDevices - 1) % numDevices;
            size_t r = (s + numDevices - 1) % numDevices;
            for (size_t step = 0; step < numDevices - 1; step++) {
                const size_t nextSlot = (slot + 1) % pipelineSize;
                const bool lastStep = step == numDevices - 2;
                char *out1 = lastStep ? recv + rank * partSize + chunkOffset : tmp + slot * chunkSize;
                char *in1 = send + r * partSize + chunkOffset;
                char *in2 = tmp + slot * chunkSize;
                char *out2 = lastStep ? nextRecv + rank * partSize + chunkOffset : nextTmp + nextSlot * chunkSize;
                if (step == 0) {
                    d.launches.push_back({Copy, {send + s * partSize + chunkOffset, nextTmp + slot * chunkSize}, 2, dataSize / elementSize});
                }
                d.launches.push_back(barrier);
                d.launches.push_back({ReduceCopy, {in1, in2, out1, out2}, 4, dataSize / elementSize});
                s = r;
                r = (s + numDevices - 1) % numDevices;
                slot = nextSlot;
            }
        }
        d.launches.push_back(barrier);

        size_t s = rank;
        for (size_t step = 1; step + 1 < numDevices; step++) {
            s = (s + numDevices - 1) % numDevices;
            d.launches.push_back({Copy, {recv + s * partSize, nextRecv + s * partSize}, 2, partSize / elementSize});
            d.launches.push_back(barrier);
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
        timestampEventPoolDesc.count = static_cast<uint32_t>(2 * numDevices);
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &timestampEventPoolDesc, static_cast<uint32_t>(numDevices), const_cast<ze_device_handle_t *>(devices.data()), &timestampEventPool));
        timestampEvents.resize(2 * numDevices);
        for (size_t i = 0; i < timestampEvents.size(); i++) {
            ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
            eventDesc.index = static_cast<uint32_t>(i);
            eventDesc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
            eventDesc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
            ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(timestampEventPool, &eventDesc, &timestampEvents[i]));
        }
    }

    for (auto &d : perDevice) {
        for (const Launch &launch : d.launches) {
            for (size_t p = 0; p < launch.numPointers; p++) {
                ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(d.kernels[launch.type], static_cast<uint32_t>(p), sizeof(void *), &launch.pointers[p]));
            }
        }
    }

    const size_t peerBytesPerDevice = throttled ? arguments.size : 2 * (numDevices - 1) * partSize;
    Timer timer;
    for (auto iteration = 0u; iteration < arguments.iterations; iteration++) {
        for (size_t i = 0; i < numDevices; i++) {
            auto &d = perDevice[i];
            const size_t lastLaunch = d.launches.size() - 1;
            for (size_t l = 0; l <= lastLaunch; l++) {
                const Launch &launch = d.launches[l];
                ze_kernel_handle_t kernel = d.kernels[launch.type];
                for (size_t p = 0; p < launch.numPointers; p++) {
                    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, static_cast<uint32_t>(p), sizeof(void *), &launch.pointers[p]));
                }
                uint32_t groups = 1u;
                if (launch.type != Barrier) {
                    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, static_cast<uint32_t>(launch.numPointers), sizeof(launch.count), &launch.count));
                    const uint64_t workItems = throttled ? static_cast<uint64_t>(arguments.throttledWorkItems) : launch.count;
                    groups = static_cast<uint32_t>((workItems + workGroupSize - 1) / workGroupSize);
                }
                const ze_group_count_t groupCount{groups, 1u, 1u};
                ze_event_handle_t signalEvent = nullptr;
                if (arguments.useEvents && l == lastLaunch) {
                    signalEvent = timestampEvents[2 * i + 1];
                } else if (arguments.useEvents && l == 0) {
                    signalEvent = timestampEvents[2 * i];
                }
                const uint32_t numWaitEvents = l == 0 ? 1u : 0u;
                ze_event_handle_t *waitEvents = l == 0 ? &startEvent : nullptr;
                ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(d.cmdList, kernel, &groupCount, signalEvent, numWaitEvents, waitEvents));
            }
        }

        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSignal(startEvent));
        for (size_t i = 0; i < numDevices; i++) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(perDevice[i].cmdList, std::numeric_limits<uint64_t>::max()));
        }
        timer.measureEnd();

        if (arguments.useEvents) {
            std::chrono::nanoseconds longestTime{0};
            for (size_t i = 0; i < numDevices; i++) {
                const bool singleLaunch = perDevice[i].launches.size() == 1;
                ze_kernel_timestamp_result_t first{};
                ze_kernel_timestamp_result_t last{};
                ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(timestampEvents[2 * i + 1], &last));
                if (singleLaunch) {
                    first = last;
                } else {
                    ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(timestampEvents[2 * i], &first));
                }
                const uint64_t ticks = BitHelper::isolateLowerNBits(last.global.kernelEnd - first.global.kernelStart, perDevice[i].timestampValidBits);
                longestTime = std::max(longestTime, std::chrono::nanoseconds(ticks * perDevice[i].timerResolution));
            }
            statistics.pushValue(longestTime, peerBytesPerDevice, typeSelector.getUnit(), typeSelector.getType());
        } else {
            statistics.pushValue(timer.get(), peerBytesPerDevice, typeSelector.getUnit(), typeSelector.getType());
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
        for (size_t k = 0; k < kernelCount; k++) {
            ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(d.kernels[k]));
        }
        ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(d.module));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(d.cmdList));
        for (void *buffer : d.buffers) {
            ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, buffer));
        }
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<UsmEUReduceCopyRing> registerTestCase(run, Api::L0);
