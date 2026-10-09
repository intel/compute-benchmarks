/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/buffer_contents_helper_l0.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/l0/utility/verification_helper_l0.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/compiler_options_builder.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/memory_constants.h"
#include "framework/utility/stream_memory_helper.h"
#include "framework/utility/timer.h"

#include "definitions/non_usm_stream_memory.h"

#include <algorithm>
#include <gtest/gtest.h>
#include <optional>

using namespace L0;

using namespace MemoryConstants;

static TestResult run(const NonUsmStreamMemoryArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::GigabytesPerSecond, arguments.useEvents ? MeasurementType::Gpu : MeasurementType::Cpu);

    if (arguments.partialMultiplier > 1u) {
        if (arguments.type == StreamMemoryType::Scale || arguments.type == StreamMemoryType::Triad) {
            return TestResult::NoImplementation;
        }
        if (arguments.type == StreamMemoryType::Write && arguments.contents != BufferContents::Zeros) {
            return TestResult::NoImplementation;
        }
    }

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    QueueProperties queueProperties = QueueProperties::create();
    ContextProperties contextProperties = ContextProperties::create();
    ExtensionProperties extensionProperties = ExtensionProperties::create();

    LevelZero levelzero(queueProperties, contextProperties, extensionProperties);
    if (levelzero.commandQueue == nullptr) {
        return TestResult::DeviceNotCapable;
    }
    if (arguments.lws > levelzero.getDeviceComputeProperties().maxGroupSizeX) {
        return TestResult::DeviceNotCapable;
    }

    if (isSharedSystemPointer(arguments.memoryPlacement)) {
        ze_device_memory_access_properties_t memoryAccessCapabilities = {};
        ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetMemoryAccessProperties(levelzero.device, &memoryAccessCapabilities));
        if (memoryAccessCapabilities.sharedSystemAllocCapabilities == 0) {
            return TestResult::DeviceNotCapable;
        }
    }
    Timer timer;

    ze_device_module_properties_t moduleProperties{};
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetModuleProperties(levelzero.device, &moduleProperties));

    size_t elementSize = arguments.vectorSize * sizeof(uint32_t);
    uint32_t scalarValue[16];
    std::fill(std::begin(scalarValue), std::end(scalarValue), static_cast<uint32_t>(-999));
    bool setScalarArgument = true;
    const auto workSize = StreamMemoryHelper::getWorkSize(arguments.size, elementSize, arguments.lws);
    if (!workSize) {
        return TestResult::InvalidArgs;
    }
    const uint32_t groupSizeX = static_cast<uint32_t>(workSize->groupSize);
    const uint32_t gws = static_cast<uint32_t>(workSize->globalSize);
    const uint64_t timerResolution = levelzero.getTimerResolution(levelzero.device);
    const bool verify = Configuration::get().verify;
    const size_t readOutputSize = verify ? workSize->bufferSize : StreamMemoryHelper::dummyOutputSize;

    const size_t bufferSize = workSize->bufferSize;
    void *buffers[3] = {};
    size_t buffersCount = {};
    size_t bufferSizes[3] = {bufferSize, bufferSize, bufferSize};
    const char *pKernelName;

    switch (arguments.type) {
    case StreamMemoryType::Read:
        pKernelName = verify ? "readWithMultiplierStore" : "readWithMultiplier";
        ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.memoryPlacement, levelzero, bufferSize, &buffers[buffersCount++]));
        bufferSizes[buffersCount] = readOutputSize;
        ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.memoryPlacement, levelzero, readOutputSize, &buffers[buffersCount++]));
        break;
    case StreamMemoryType::Write:
        if (BufferContents::Random == arguments.contents) {
            pKernelName = "write_random";
            setScalarArgument = false; // value to write is embedded in kernel code
        } else {
            pKernelName = "writeWithMultiplier";
        }
        ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.memoryPlacement, levelzero, bufferSize, &buffers[buffersCount++]));
        break;
    case StreamMemoryType::Scale:
        pKernelName = "scale";
        ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.memoryPlacement, levelzero, bufferSize, &buffers[buffersCount++]));
        ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.memoryPlacement, levelzero, bufferSize, &buffers[buffersCount++]));
        break;
    case StreamMemoryType::Triad:
        pKernelName = "triad";
        ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.memoryPlacement, levelzero, bufferSize, &buffers[buffersCount++]));
        ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.memoryPlacement, levelzero, bufferSize, &buffers[buffersCount++]));
        ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.memoryPlacement, levelzero, bufferSize, &buffers[buffersCount++]));
        break;
    default:
        FATAL_ERROR("Unknown StreamMemoryType");
    }

    ze_kernel_handle_t kernel{};
    ze_module_handle_t module{};
    CompilerOptionsBuilder compilerOptions;
    std::string streamType = "uint";
    if (arguments.vectorSize > 1) {
        streamType += std::to_string(arguments.vectorSize);
    }
    compilerOptions.addDefinitionKeyValue("STREAM_TYPE", streamType.c_str());
    compilerOptions.addDefinitionKeyValue("WRITE_RANDOM_PATTERN", StreamMemoryHelper::getWriteRandomPatternDefinition());

    auto kernelLoadRes = L0::KernelHelper::loadKernel(levelzero, "memory_benchmark_stream_memory.cl", pKernelName, &kernel, &module, compilerOptions.str().c_str());
    if (kernelLoadRes != TestResult::Success) {
        return kernelLoadRes;
    }

    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, groupSizeX, 1u, 1u));
    const ze_group_count_t dispatchTraits{gws / groupSizeX, 1u, 1u};

    ze_command_list_handle_t cmdList;
    ze_command_list_desc_t cmdListDesc{ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));

    ze_event_pool_flags_t eventPoolFlags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
    if (arguments.useEvents) {
        eventPoolFlags |= ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP;
    }
    ze_event_pool_handle_t eventPool{};
    ze_event_handle_t event{};
    ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
    eventPoolDesc.flags = eventPoolFlags;
    eventPoolDesc.count = 1;
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.commandQueueDevice, &eventPool));
    ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
    eventDesc.index = 0;
    eventDesc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
    eventDesc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
    ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &event));

    const bool systemMemory = isSharedSystemPointer(arguments.memoryPlacement);
    const size_t outputIndex = buffersCount - 1;
    const bool invertSecondInput = verify && arguments.type == StreamMemoryType::Triad && arguments.contents == BufferContents::Random;
    uint8_t systemFillBytes[3] = {};
    auto fillSystemBuffers = [&]() {
        for (auto id = 0u; id < buffersCount; id++) {
            systemFillBytes[id] = arguments.contents == BufferContents::Zeros ? 0u : static_cast<uint8_t>(rand() & 0xff);
            if (verify && id == outputIndex) {
                systemFillBytes[id] = VerificationHelper::poisonByte;
            } else if (invertSecondInput && id == 1) {
                systemFillBytes[id] = static_cast<uint8_t>(~systemFillBytes[0]);
            }
            memset(buffers[id], systemFillBytes[id], bufferSizes[id]);
        }
    };
    if (systemMemory) {
        fillSystemBuffers();
    }
    for (auto i = 0u; i < buffersCount; i++) {
        if (!systemMemory) {
            if (invertSecondInput && i == 1) {
                const auto y = StreamMemoryHelper::getInput(arguments.contents, 1);
                ASSERT_ZE_RESULT_SUCCESS(BufferContentsHelperL0::fillBufferWithData(levelzero, buffers[i], bufferSizes[i], [&](uint8_t *data) { y.copy(data, 0, bufferSizes[i]); }, false));
            } else {
                ASSERT_ZE_RESULT_SUCCESS(BufferContentsHelperL0::fillBuffer(levelzero, buffers[i], bufferSizes[i], arguments.contents, false));
            }
        }
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, static_cast<int>(i), sizeof(buffers[i]), &buffers[i]));
    }
    if (setScalarArgument) {
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, static_cast<uint32_t>(buffersCount), elementSize, scalarValue));
    }

    int multiplier = static_cast<int>(arguments.partialMultiplier);
    if ((arguments.type == StreamMemoryType::Write && arguments.contents != BufferContents::Random) || arguments.type == StreamMemoryType::Read) {
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, static_cast<uint32_t>(setScalarArgument ? buffersCount + 1 : buffersCount), 4u, &multiplier));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, 0));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListReset(cmdList));

    if (arguments.prefetch) {
        for (auto i = 0u; i < buffersCount; i++) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryPrefetch(cmdList, buffers[i], bufferSizes[i]));
        }
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &dispatchTraits, event, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

    VerificationHelperL0 verification(levelzero);
    std::optional<StreamMemoryHelper::Verifier> verifier;
    if (verify) {
        if (!systemMemory) {
            ASSERT_ZE_RESULT_SUCCESS(verification.initialize(bufferSize));
        }
        verifier.emplace(arguments.type, arguments.contents, bufferSize, elementSize, arguments.partialMultiplier, scalarValue[0]);
    }
    const auto readChunk = [&](size_t offset, size_t size) -> const uint8_t * {
        if (systemMemory) {
            return static_cast<const uint8_t *>(buffers[outputIndex]) + offset;
        }
        return verification.read(buffers[outputIndex], offset, size);
    };
    TestResult result = TestResult::Success;

    for (auto i = 0u; i < arguments.iterations; i++) {
        if (systemMemory) {
            fillSystemBuffers();
        } else if (verify) {
            ASSERT_ZE_RESULT_SUCCESS(verification.poison(buffers[outputIndex], bufferSizes[outputIndex]));
        }
        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, 0));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
        timer.measureEnd();

        size_t transferSize = bufferSize;
        switch (arguments.type) {
        case StreamMemoryType::Scale:
            transferSize *= 2;
            break;
        case StreamMemoryType::Triad:
            transferSize *= 3;
            break;
        default:
            break;
        }

        if (arguments.partialMultiplier > 1u) {
            transferSize = transferSize / arguments.partialMultiplier;
        }

        if (arguments.useEvents) {
            ze_kernel_timestamp_result_t timestampResult{};
            ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(event, &timestampResult));
            auto commandTime = std::chrono::nanoseconds(timestampResult.global.kernelEnd - timestampResult.global.kernelStart);
            commandTime *= timerResolution;
            statistics.pushValue(commandTime, transferSize, typeSelector.getUnit(), typeSelector.getType());
        } else {
            statistics.pushValue(timer.get(), transferSize, typeSelector.getUnit(), typeSelector.getType());
        }
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(event));

        if (verify) {
            if (systemMemory) {
                verifier->setInputs({.fill = systemFillBytes[0]}, {.fill = systemFillBytes[1]});
            }
            result = verifier->verify(readChunk);
            if (result != TestResult::Success) {
                break;
            }
        }
    }

    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    for (size_t i = 0; i < buffersCount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(arguments.memoryPlacement, levelzero, buffers[i]));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));

    return result;
}

static RegisterTestCaseImplementation<NonUsmStreamMemory> registerTestCase(run, Api::L0);
