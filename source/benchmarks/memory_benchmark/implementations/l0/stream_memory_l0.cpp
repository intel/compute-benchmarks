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
#include "framework/test_case/register_test_case.h"
#include "framework/utility/compiler_options_builder.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/memory_constants.h"
#include "framework/utility/timer.h"

#include "definitions/stream_memory.h"
#include "stream_memory_verification.h"

#include <algorithm>
#include <gtest/gtest.h>

using namespace L0;

using namespace MemoryConstants;

static TestResult run(const StreamMemoryArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::GigabytesPerSecond, arguments.useEvents ? MeasurementType::Gpu : MeasurementType::Cpu);

    if (isStreamMemoryUnsupported(arguments)) {
        return TestResult::NoImplementation;
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

    Timer timer;

    const auto scalarValue = getStreamMemoryScalarValue();

    StreamMemoryDispatch dispatch{};
    if (const TestResult result = getStreamMemoryDispatch(arguments, dispatch); result != TestResult::Success) {
        return result;
    }
    const uint64_t timerResolution = levelzero.getTimerResolution(levelzero.device);

    void *buffers[3] = {};
    size_t buffersCount = {};
    size_t bufferSizes[3] = {dispatch.bufferSize, dispatch.bufferSize, dispatch.bufferSize};
    const char *pKernelName = getStreamMemoryKernelName(arguments);
    const bool setScalarArgument = streamMemoryHasScalarArgument(arguments);

    switch (arguments.type) {
    case StreamMemoryType::Read:
        ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.memoryPlacement, levelzero, dispatch.bufferSize, &buffers[buffersCount++]));
        bufferSizes[buffersCount] = 16u;
        ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.memoryPlacement, levelzero, 16u, &buffers[buffersCount++]));
        break;
    case StreamMemoryType::Triad:
        ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.memoryPlacement, levelzero, dispatch.bufferSize, &buffers[buffersCount++]));
        [[fallthrough]];
    case StreamMemoryType::Scale:
        ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.memoryPlacement, levelzero, dispatch.bufferSize, &buffers[buffersCount++]));
        [[fallthrough]];
    case StreamMemoryType::Write:
        ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.memoryPlacement, levelzero, dispatch.bufferSize, &buffers[buffersCount++]));
        break;
    default:
        FATAL_ERROR("Unknown StreamMemoryType");
    }

    ze_kernel_handle_t kernel{};
    ze_module_handle_t module{};
    CompilerOptionsBuilder compilerOptions;
    compilerOptions.addDefinitionKeyValue("STREAM_TYPE", getStreamMemoryTypeDefinition(arguments.vectorSize));
    compilerOptions.addDefinitionKeyValue("WRITE_RANDOM_PATTERN", StreamMemoryVerification::getWriteRandomPatternDefinition());

    auto kernelLoadRes = L0::KernelHelper::loadKernel(levelzero, "memory_benchmark_stream_memory.cl", pKernelName, &kernel, &module, compilerOptions.str().c_str());
    if (kernelLoadRes != TestResult::Success) {
        return kernelLoadRes;
    }

    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, static_cast<uint32_t>(dispatch.groupSize), 1u, 1u));
    const ze_group_count_t dispatchTraits{static_cast<uint32_t>(dispatch.groupCount), 1u, 1u};

    ze_command_list_handle_t cmdList;
    ze_command_list_desc_t cmdListDesc{};
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

    const bool verify = Configuration::get().verify;
    const size_t outputIndex = arguments.type == StreamMemoryType::Read ? 0u : buffersCount - 1;
    const bool poisonOutput = verify && (arguments.type == StreamMemoryType::Scale || arguments.type == StreamMemoryType::Triad);
    for (auto i = 0u; i < buffersCount; i++) {
        if (poisonOutput && i == outputIndex) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryFill(cmdList, buffers[i], &StreamMemoryVerification::outputPoison, 1u, bufferSizes[i], nullptr, 0, nullptr));
        } else if (verify && arguments.type == StreamMemoryType::Triad && i == 1 && arguments.contents == BufferContents::Random) {
            const auto y = StreamMemoryVerification::getInput(arguments.contents, bufferSizes[i], 1);
            ASSERT_ZE_RESULT_SUCCESS(BufferContentsHelperL0::fillBufferWithData(levelzero, buffers[i], bufferSizes[i], [&](uint8_t *data) { y.copy(data, 0, bufferSizes[i]); }, true));
        } else {
            ASSERT_ZE_RESULT_SUCCESS(BufferContentsHelperL0::fillBuffer(levelzero, buffers[i], bufferSizes[i], arguments.contents, true));
        }
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, static_cast<int>(i), sizeof(buffers[i]), &buffers[i]));
    }
    if (setScalarArgument) {
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, static_cast<uint32_t>(buffersCount), dispatch.elementSize, scalarValue.data()));
    }

    int multiplier = static_cast<int>(arguments.partialMultiplier);
    if ((arguments.type == StreamMemoryType::Write && arguments.contents != BufferContents::Random) || arguments.type == StreamMemoryType::Read) {
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, static_cast<uint32_t>(setScalarArgument ? buffersCount + 1 : buffersCount), 4u, &multiplier));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, 0));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListReset(cmdList));

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &dispatchTraits, event, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

    ze_command_list_handle_t copyCmdList{};
    void *staging{};
    if (verify) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &copyCmdList));
        ze_host_mem_alloc_desc_t hostDesc{ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC};
        ASSERT_ZE_RESULT_SUCCESS(zeMemAllocHost(levelzero.context, &hostDesc, std::min(dispatch.bufferSize, VerificationHelper::chunkSize), 0, &staging));
    }
    const auto readChunk = [&](size_t offset, size_t size) -> const uint8_t * {
        ZE_RESULT_SUCCESS_OR_RETURN_VALUE(BufferContentsHelperL0::copyToHost(levelzero, copyCmdList, staging, static_cast<uint8_t *>(buffers[outputIndex]) + offset, size), nullptr);
        return static_cast<const uint8_t *>(staging);
    };
    const auto expected = StreamMemoryVerification::getExpected(arguments.type, arguments.contents, dispatch.bufferSize, dispatch.elementSize, arguments.partialMultiplier, scalarValue[0]);
    TestResult result = TestResult::Success;

    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, 0));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
        timer.measureEnd();

        const size_t transferSize = getStreamMemoryTransferSize(arguments, dispatch.bufferSize);

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
            result = StreamMemoryVerification::verifyOutput(expected, dispatch.bufferSize, readChunk);
            if (result != TestResult::Success) {
                break;
            }
        }
    }

    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    for (size_t i = 0; i < buffersCount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, buffers[i]));
    }
    if (verify) {
        ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, staging));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(copyCmdList));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));

    return result;
}

static RegisterTestCaseImplementation<StreamMemory> registerTestCase(run, Api::L0);
