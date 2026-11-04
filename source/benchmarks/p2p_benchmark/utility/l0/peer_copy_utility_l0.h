/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/configuration.h"
#include "framework/enum/engine.h"
#include "framework/l0/levelzero.h"
#include "framework/l0/utility/buffer_contents_helper_l0.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/l0/utility/queue_families_helper.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/test_result.h"
#include "framework/utility/statistics.h"
#include "framework/utility/timer.h"

#include "definitions/peer_copy_arguments.h"

#include <algorithm>
#include <level_zero/ze_api.h>
#include <string>
#include <vector>

enum class PeerCopySubmitter {
    Local,
    Peer,
};

enum class PeerCopyDirection {
    Write,
    Read,
};

struct PeerCopy {
    PeerCopySubmitter submitter = PeerCopySubmitter::Local;
    PeerCopyDirection direction = PeerCopyDirection::Write;
    Engine engine = Engine::Unknown;
    ze_device_handle_t submittingDevice{};
    ze_device_handle_t copySourceDevice{};
    ze_device_handle_t copyDestinationDevice{};
    ze_command_queue_desc_t queueDesc{};
    ze_command_list_handle_t cmdList{};
    ze_module_handle_t module{};
    ze_kernel_handle_t kernel{};
    void *source{};
    void *destination{};
    void *reduceInput{};
    void *reduceOutput{};
    ze_event_handle_t startEvent{};
    ze_event_handle_t completionEvent{};
    Timer timer{};
    bool done = false;

    bool isSubmittedByLocal() const {
        return this->submitter == PeerCopySubmitter::Local;
    }

    bool isWrite() const {
        return this->direction == PeerCopyDirection::Write;
    }

    std::string getLabel() const {
        return std::string(this->isSubmittedByLocal() ? "Local" : "Peer") + (this->isWrite() ? "Write" : "Read");
    }
};

enum class PeerCopyKernel {
    None,
    Copy,
    ThrottledCopy,
    ReduceCopy,
    ThrottledReduceCopy,
};

inline bool isReduceCopyKernel(PeerCopyKernel kernel) {
    return kernel == PeerCopyKernel::ReduceCopy || kernel == PeerCopyKernel::ThrottledReduceCopy;
}

inline bool isThrottledKernel(PeerCopyKernel kernel) {
    return kernel == PeerCopyKernel::ThrottledCopy || kernel == PeerCopyKernel::ThrottledReduceCopy;
}

struct PeerCopyOptions {
    PeerCopyKernel kernel = PeerCopyKernel::None;
    size_t chunkSize = 0;
    size_t throttledWorkItems = 0;
};

inline TestResult allocatePeerCopyBuffer(L0::LevelZero &levelzero, ze_device_handle_t device, size_t size, void **buffer) {
    const ze_result_t result = L0::UsmHelper::allocate(UsmRuntimeMemoryPlacement::Device, levelzero, device, size, buffer);
    if (result == ZE_RESULT_ERROR_OUT_OF_DEVICE_MEMORY || result == ZE_RESULT_ERROR_UNSUPPORTED_SIZE) {
        return TestResult::DeviceNotCapable;
    }
    ASSERT_ZE_RESULT_SUCCESS(result);
    return TestResult::Success;
}

struct PeerCopyKernelSource {
    const char *file;
    const char *name;
    const char *buildFlags;
};

inline PeerCopyKernelSource getPeerCopyKernelSource(PeerCopyKernel kernel) {
    switch (kernel) {
    case PeerCopyKernel::Copy:
        return {"p2p_benchmark_reduce_copy.cl", "copy", "-cl-std=CL3.0"};
    case PeerCopyKernel::ThrottledCopy:
        return {"p2p_benchmark_copy_throttled.cl", "copy_throttled", nullptr};
    case PeerCopyKernel::ReduceCopy:
        return {"p2p_benchmark_reduce_copy.cl", "reduce_copy", "-cl-std=CL3.0"};
    default:
        return {"p2p_benchmark_reduce_copy_throttled.cl", "reduce_copy_throttled", nullptr};
    }
}

inline TestResult appendPeerCopy(const PeerCopy &copy, const PeerCopyOptions &options, size_t offset, size_t copySize,
                                 ze_event_handle_t signalEvent, uint32_t numWaitEvents, ze_event_handle_t *waitEvents) {
    void *destination = static_cast<char *>(copy.destination) + offset;
    const void *source = static_cast<const char *>(copy.source) + offset;
    if (options.kernel == PeerCopyKernel::None) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(copy.cmdList, destination, source, copySize, signalEvent, numWaitEvents, waitEvents));
        return TestResult::Success;
    }
    const uint64_t elementCount = copySize / PeerCopyArguments::reduceCopyElementSize;
    const uint64_t workItems = isThrottledKernel(options.kernel) ? options.throttledWorkItems : elementCount;
    const ze_group_count_t dispatch{static_cast<uint32_t>((workItems + PeerCopyArguments::reduceCopyWorkGroupSize - 1) / PeerCopyArguments::reduceCopyWorkGroupSize), 1u, 1u};
    uint32_t argumentIndex = 0;
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(copy.kernel, argumentIndex++, sizeof(source), &source));
    if (isReduceCopyKernel(options.kernel)) {
        const void *reduceInput = static_cast<const char *>(copy.reduceInput) + offset;
        void *reduceOutput = static_cast<char *>(copy.reduceOutput) + offset;
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(copy.kernel, argumentIndex++, sizeof(reduceInput), &reduceInput));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(copy.kernel, argumentIndex++, sizeof(reduceOutput), &reduceOutput));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(copy.kernel, argumentIndex++, sizeof(destination), &destination));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(copy.kernel, argumentIndex, sizeof(elementCount), &elementCount));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(copy.cmdList, copy.kernel, &dispatch, signalEvent, numWaitEvents, waitEvents));
    return TestResult::Success;
}

inline TestResult measurePeerCopies(const PeerCopyArguments &arguments, std::vector<PeerCopy> copies, const PeerCopyOptions &options, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::GigabytesPerSecond, arguments.useEvents ? MeasurementType::Gpu : MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    L0::LevelZero levelzero(L0::QueueProperties::create().disable(), L0::ContextProperties::create());

    const size_t copySize = arguments.size;
    const size_t chunkSize = options.chunkSize == 0 ? copySize : std::min(options.chunkSize, copySize);
    const size_t chunkCount = (copySize + chunkSize - 1) / chunkSize;
    const bool measureChunkSpan = arguments.useEvents && chunkCount > 1;

    const size_t deviceCount = levelzero.rootDevices.size();
    if (static_cast<size_t>(arguments.localDeviceId) >= deviceCount || static_cast<size_t>(arguments.peerDeviceId) >= deviceCount) {
        return TestResult::DeviceNotCapable;
    }
    const ze_device_handle_t localDevice = levelzero.rootDevices[arguments.localDeviceId];
    const ze_device_handle_t peerDevice = levelzero.rootDevices[arguments.peerDeviceId];

    for (PeerCopy &copy : copies) {
        copy.submittingDevice = copy.isSubmittedByLocal() ? localDevice : peerDevice;
        const ze_device_handle_t remoteDevice = copy.isSubmittedByLocal() ? peerDevice : localDevice;
        copy.copySourceDevice = copy.isWrite() ? copy.submittingDevice : remoteDevice;
        copy.copyDestinationDevice = copy.isWrite() ? remoteDevice : copy.submittingDevice;

        ze_bool_t canAccessPeer = false;
        ASSERT_ZE_RESULT_SUCCESS(zeDeviceCanAccessPeer(copy.submittingDevice, remoteDevice, &canAccessPeer));
        if (!canAccessPeer) {
            return TestResult::DeviceNotCapable;
        }

        const auto queueDesc = L0::QueueFamiliesHelper::getPropertiesForSelectingEngine(copy.submittingDevice, copy.engine);
        if (queueDesc == nullptr) {
            return TestResult::DeviceNotCapable;
        }
        copy.queueDesc = queueDesc->desc;
        if (chunkCount > 1) {
            copy.queueDesc.flags |= ZE_COMMAND_QUEUE_FLAG_IN_ORDER;
        }

        if (options.kernel != PeerCopyKernel::None) {
            const PeerCopyKernelSource kernelSource = getPeerCopyKernelSource(options.kernel);
            std::string buildFlags = kernelSource.buildFlags != nullptr ? kernelSource.buildFlags : "";
            if (copySize >= PeerCopyArguments::statelessBufferSize) {
                buildFlags += " -ze-opt-greater-than-4GB-buffer-required";
            }
            ASSERT_TEST_RESULT_SUCCESS(L0::KernelHelper::loadKernel(levelzero, copy.submittingDevice, kernelSource.file, kernelSource.name,
                                                                    &copy.kernel, &copy.module, buildFlags.c_str(), 0));
            ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(copy.kernel, PeerCopyArguments::reduceCopyWorkGroupSize, 1u, 1u));
        }
    }

    ze_event_pool_handle_t gateEventPool{};
    ze_event_pool_desc_t gateEventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
    gateEventPoolDesc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
    gateEventPoolDesc.count = 1;
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &gateEventPoolDesc, 0, nullptr, &gateEventPool));
    ze_event_handle_t gateEvent{};
    ze_event_desc_t gateEventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
    gateEventDesc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
    gateEventDesc.wait = ZE_EVENT_SCOPE_FLAG_DEVICE;
    ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(gateEventPool, &gateEventDesc, &gateEvent));

    ze_event_pool_handle_t completionEventPool{};
    ze_event_pool_desc_t completionEventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
    completionEventPoolDesc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
    if (arguments.useEvents) {
        completionEventPoolDesc.flags |= ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP;
    }
    const size_t eventsPerCopy = measureChunkSpan ? 2 : 1;
    completionEventPoolDesc.count = static_cast<uint32_t>(eventsPerCopy * copies.size());
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &completionEventPoolDesc, 0, nullptr, &completionEventPool));

    for (auto i = 0u; i < copies.size(); i++) {
        PeerCopy &copy = copies[i];
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, copy.submittingDevice, &copy.queueDesc, &copy.cmdList));

        ASSERT_TEST_RESULT_SUCCESS(allocatePeerCopyBuffer(levelzero, copy.copySourceDevice, arguments.size, &copy.source));
        ASSERT_TEST_RESULT_SUCCESS(allocatePeerCopyBuffer(levelzero, copy.copyDestinationDevice, arguments.size, &copy.destination));
        ASSERT_ZE_RESULT_SUCCESS(BufferContentsHelperL0::fillBuffer(copy.copySourceDevice, levelzero.context, nullptr, 0, copy.source, arguments.size, BufferContents::Random, true));
        if (isReduceCopyKernel(options.kernel)) {
            ASSERT_TEST_RESULT_SUCCESS(allocatePeerCopyBuffer(levelzero, copy.submittingDevice, arguments.size, &copy.reduceInput));
            ASSERT_TEST_RESULT_SUCCESS(allocatePeerCopyBuffer(levelzero, copy.submittingDevice, arguments.size, &copy.reduceOutput));
            ASSERT_ZE_RESULT_SUCCESS(BufferContentsHelperL0::fillBuffer(copy.submittingDevice, levelzero.context, nullptr, 0, copy.reduceInput, arguments.size, BufferContents::Random, true));
        }

        ze_event_desc_t completionEventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
        completionEventDesc.index = static_cast<uint32_t>(eventsPerCopy * i);
        completionEventDesc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
        completionEventDesc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
        ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(completionEventPool, &completionEventDesc, &copy.completionEvent));
        if (measureChunkSpan) {
            completionEventDesc.index++;
            ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(completionEventPool, &completionEventDesc, &copy.startEvent));
        }
    }

    Timer timer;
    for (auto i = 0u; i < arguments.iterations; i++) {
        for (PeerCopy &copy : copies) {
            for (size_t chunk = 0; chunk < chunkCount; chunk++) {
                const size_t offset = chunk * chunkSize;
                const bool firstChunk = chunk == 0;
                ze_event_handle_t signalEvent = nullptr;
                if (chunk + 1 == chunkCount) {
                    signalEvent = copy.completionEvent;
                } else if (firstChunk) {
                    signalEvent = copy.startEvent;
                }
                ASSERT_TEST_RESULT_SUCCESS(appendPeerCopy(copy, options, offset, std::min(chunkSize, copySize - offset),
                                                          signalEvent, firstChunk ? 1u : 0u, firstChunk ? &gateEvent : nullptr));
            }
            copy.done = false;
        }

        timer.measureStart();
        for (PeerCopy &copy : copies) {
            copy.timer.measureStart();
        }
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSignal(gateEvent));
        size_t pendingCopies = copies.size();
        while (pendingCopies > 0) {
            for (PeerCopy &copy : copies) {
                if (copy.done) {
                    continue;
                }
                const ze_result_t status = zeEventQueryStatus(copy.completionEvent);
                if (status == ZE_RESULT_NOT_READY) {
                    continue;
                }
                ASSERT_ZE_RESULT_SUCCESS(status);
                copy.timer.measureEnd();
                copy.done = true;
                pendingCopies--;
            }
        }
        timer.measureEnd();

        if (copies.size() > 1) {
            // Rows print sorted by label and CI tracks the first row, so this label sorts before the per-copy ones.
            statistics.pushValue(timer.get(), arguments.size * copies.size(), typeSelector.getUnit(), MeasurementType::Cpu, "AllCopies");
        }
        for (PeerCopy &copy : copies) {
            std::chrono::nanoseconds copyTime = copy.timer.get();
            if (arguments.useEvents) {
                ze_kernel_timestamp_result_t timestamp{};
                ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(copy.completionEvent, &timestamp));
                ze_kernel_timestamp_data_t copySpan = timestamp.global;
                if (measureChunkSpan) {
                    ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(copy.startEvent, &timestamp));
                    copySpan.kernelStart = timestamp.global.kernelStart;
                    ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(copy.startEvent));
                }
                copyTime = levelzero.getAbsoluteKernelExecutionTime(copySpan, copy.submittingDevice);
            }
            if (copies.size() > 1) {
                statistics.pushValue(copyTime, arguments.size, typeSelector.getUnit(), typeSelector.getType(), copy.getLabel());
            } else {
                statistics.pushValue(copyTime, arguments.size, typeSelector.getUnit(), typeSelector.getType());
            }
            ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(copy.completionEvent));
        }
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(gateEvent));
    }

    for (PeerCopy &copy : copies) {
        if (copy.startEvent != nullptr) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(copy.startEvent));
        }
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(copy.completionEvent));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(copy.cmdList));
        if (copy.kernel != nullptr) {
            ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(copy.kernel));
            ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(copy.module));
        }
        if (copy.reduceInput != nullptr) {
            ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, copy.reduceInput));
            ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, copy.reduceOutput));
        }
        ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, copy.source));
        ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, copy.destination));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(gateEvent));
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(completionEventPool));
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(gateEventPool));
    return TestResult::Success;
}

inline TestResult measurePeerCopies(const PeerCopyArguments &arguments, std::vector<PeerCopy> copies, Statistics &statistics) {
    return measurePeerCopies(arguments, std::move(copies), PeerCopyOptions{}, statistics);
}
