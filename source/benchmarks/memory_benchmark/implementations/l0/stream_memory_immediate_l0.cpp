/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/l0/utility/verification_helper_l0.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/compiler_options_builder.h"
#include "framework/utility/memory_constants.h"
#include "framework/utility/stream_memory_helper.h"
#include "framework/utility/timer.h"
#include "framework/utility/verification_helper.h"

#include "definitions/stream_memory_immediate.h"

#include <gtest/gtest.h>

using namespace L0;

using namespace MemoryConstants;

template <typename T>
static T getExpectedValue(StreamMemoryType type, const T *values) {
    switch (type) {
    case StreamMemoryType::Read:
        return values[0];
    case StreamMemoryType::Write:
        return values[2];
    case StreamMemoryType::Scale:
        return values[0] * values[2];
    case StreamMemoryType::Triad:
        return values[0] + values[1] * values[2];
    default:
        FATAL_ERROR("Unknown StreamMemoryType");
    }
}

static TestResult run(const StreamMemoryImmediateArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::GigabytesPerSecond, arguments.useEvents ? MeasurementType::Gpu : MeasurementType::Cpu);

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
    Timer timer;

    ze_device_module_properties_t moduleProperties{};
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetModuleProperties(levelzero.device, &moduleProperties));

    const bool useDoubles = moduleProperties.fp64flags != 0u;
    const size_t elementSize = useDoubles ? sizeof(double) : sizeof(float);
    const float floatValues[] = {313.f, 17.f, -999.f};
    const double doubleValues[] = {313.0, 17.0, -999.0};
    const void *fillValue = useDoubles ? static_cast<const void *>(&doubleValues[0]) : &floatValues[0];
    const void *secondFillValue = useDoubles ? static_cast<const void *>(&doubleValues[1]) : &floatValues[1];
    const void *scalarValue = useDoubles ? static_cast<const void *>(&doubleValues[2]) : &floatValues[2];
    const auto workSize = StreamMemoryHelper::getWorkSize(arguments.size, elementSize, levelzero.getDeviceComputeProperties().maxGroupSizeX);
    if (!workSize) {
        return TestResult::InvalidArgs;
    }
    const uint32_t groupSizeX = static_cast<uint32_t>(workSize->groupSize);
    const uint32_t gws = static_cast<uint32_t>(workSize->globalSize);
    const uint64_t timerResolution = levelzero.getTimerResolution(levelzero.device);
    const auto verifyMode = StreamMemoryHelper::getVerification(Configuration::get().verify, Configuration::get().verificationMode);
    const bool verify = verifyMode != StreamMemoryHelper::Verification::None;
    const bool lightVerification = verifyMode == StreamMemoryHelper::Verification::Light;
    const bool gatherSamples = lightVerification && arguments.type != StreamMemoryType::Read;
    const size_t samplesSize = StreamMemoryHelper::getSamplesSize(workSize->bufferSize, elementSize, workSize->groupSize);
    const size_t readOutputSize = StreamMemoryHelper::getReadOutputSize(verifyMode, workSize->bufferSize, samplesSize);

    const size_t bufferSize = workSize->bufferSize;
    void *buffers[3] = {};
    size_t buffersCount = {};
    size_t bufferSizes[3] = {bufferSize, bufferSize, bufferSize};

    const ze_device_mem_alloc_desc_t deviceAllocationDesc{ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC};
    const char *pKernelName = nullptr;
    switch (arguments.type) {
    case StreamMemoryType::Read:
        pKernelName = lightVerification ? "readSample" : (verify ? "readStore" : "read");
        ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &deviceAllocationDesc, bufferSize, 0, levelzero.device, &buffers[buffersCount++]));
        bufferSizes[buffersCount] = readOutputSize;
        ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &deviceAllocationDesc, readOutputSize, 0, levelzero.device, &buffers[buffersCount++]));
        break;
    case StreamMemoryType::Write:
        pKernelName = "write";
        ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &deviceAllocationDesc, bufferSize, 0, levelzero.device, &buffers[buffersCount++]));
        break;
    case StreamMemoryType::Scale:
        pKernelName = "scale";
        ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &deviceAllocationDesc, bufferSize, 0, levelzero.device, &buffers[buffersCount++]));
        ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &deviceAllocationDesc, bufferSize, 0, levelzero.device, &buffers[buffersCount++]));
        break;
    case StreamMemoryType::Triad:
        pKernelName = "triad";
        ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &deviceAllocationDesc, bufferSize, 0, levelzero.device, &buffers[buffersCount++]));
        ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &deviceAllocationDesc, bufferSize, 0, levelzero.device, &buffers[buffersCount++]));
        ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &deviceAllocationDesc, bufferSize, 0, levelzero.device, &buffers[buffersCount++]));
        break;
    default:
        FATAL_ERROR("Unknown StreamMemoryType");
    }

    ze_kernel_handle_t kernel{};
    ze_module_handle_t module{};
    CompilerOptionsBuilder compilerOptions;
    compilerOptions.addDefinitionKeyValue("STREAM_TYPE", useDoubles ? "double" : "float");
    compilerOptions.addDefinitionKeyValue("SAMPLE_GROUP_STRIDE", StreamMemoryHelper::getSampleGroupStrideDefinition());
    auto kernelLoadRes = L0::KernelHelper::loadKernel(levelzero, "memory_benchmark_stream_memory.cl", pKernelName, &kernel, &module, compilerOptions.str().c_str());
    if (kernelLoadRes != TestResult::Success) {
        return kernelLoadRes;
    }

    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, groupSizeX, 1u, 1u));
    const ze_group_count_t dispatchTraits{gws / groupSizeX, 1u, 1u};

    ze_command_list_handle_t cmdList;
    ze_command_queue_desc_t commandQueueDesc = levelzero.commandQueueDesc;
    commandQueueDesc.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc, &cmdList));

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
    eventDesc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
    eventDesc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
    ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &event));

    for (auto i = 0u; i < buffersCount; i++) {
        const void *pattern = verify && arguments.type == StreamMemoryType::Triad && i == 1 ? secondFillValue : fillValue;
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryFill(cmdList, buffers[i], pattern, elementSize, bufferSizes[i], event, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, static_cast<int>(i), sizeof(buffers[i]), &buffers[i]));
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event, std::numeric_limits<uint64_t>::max()));
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(event));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, static_cast<uint32_t>(buffersCount), elementSize, scalarValue));

    const size_t outputIndex = buffersCount - 1;
    void *samples = arguments.type == StreamMemoryType::Read ? buffers[outputIndex] : nullptr;
    VerificationHelperL0 verification(levelzero);
    if (verify) {
        ASSERT_ZE_RESULT_SUCCESS(verification.initialize(lightVerification ? samplesSize : bufferSize));
    }
    if (gatherSamples) {
        ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &deviceAllocationDesc, samplesSize, 0, levelzero.device, &samples));
        if (const TestResult gatherResult = verification.initializeSampling("memory_benchmark_stream_memory.cl", compilerOptions.str().c_str(), buffers[outputIndex], samples,
                                                                            samplesSize / elementSize, gws, groupSizeX, 1u);
            gatherResult != TestResult::Success) {
            return gatherResult;
        }
    }
    const void *checkedBuffer = lightVerification ? samples : buffers[outputIndex];
    const size_t checkedSize = lightVerification ? samplesSize : bufferSize;
    const auto readChunk = [&](size_t offset, size_t size) -> const void * { return verification.read(checkedBuffer, offset, size); };
    TestResult result = TestResult::Success;

    for (auto i = 0u; i < arguments.iterations; i++) {
        if (verifyMode == StreamMemoryHelper::Verification::Complete) {
            ASSERT_ZE_RESULT_SUCCESS(verification.poison(buffers[outputIndex], bufferSizes[outputIndex]));
        } else if (gatherSamples) {
            ASSERT_ZE_RESULT_SUCCESS(verification.resetSamples());
        } else if (lightVerification) {
            ASSERT_ZE_RESULT_SUCCESS(verification.poison(samples, samplesSize));
        }
        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &dispatchTraits, event, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event, std::numeric_limits<uint64_t>::max()));
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

        if (gatherSamples) {
            ASSERT_ZE_RESULT_SUCCESS(verification.gather());
        }
        if (verify) {
            result = useDoubles ? VerificationHelper::verifyUniformOutput(checkedSize, getExpectedValue(arguments.type, doubleValues), readChunk)
                                : VerificationHelper::verifyUniformOutput(checkedSize, getExpectedValue(arguments.type, floatValues), readChunk);
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
    if (gatherSamples) {
        ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, samples));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));

    return result;
}

static RegisterTestCaseImplementation<StreamMemoryImmediate> registerTestCase(run, Api::L0);
