/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/graph_with_transfers.h"

#include <cmath>
#include <cstring>
#include <level_zero/zer_api.h>
#include <limits>
#include <numeric>
#include <vector>

using namespace L0;

static constexpr size_t computeElementsCount = 1024 * 1024;
static constexpr uint32_t computeKernelCount = 3;
static constexpr size_t computeVerificationStride = 4096;

static float computeBoundReference(float src, uint32_t iterations) {
    float a = src;
    float b = a + 1.0f;
    float c = a + 2.0f;
    float d = a + 3.0f;
    for (uint32_t i = 0; i < iterations; i++) {
        a = std::fma(a, 0.999f, 0.001f);
        b = std::fma(b, 0.998f, 0.002f);
        c = std::fma(c, 0.997f, 0.003f);
        d = std::fma(d, 0.996f, 0.004f);
    }
    return a + b + c + d;
}

static TestResult run(const GraphWithTransfersArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    if (arguments.size % sizeof(uint32_t) != 0) {
        return TestResult::InvalidArgs;
    }

    ExtensionProperties extensionProperties = ExtensionProperties::create()
                                                  .setGraphFunctions(true);
    LevelZero levelzero(extensionProperties);

    const size_t size = arguments.size;
    const size_t elementsCount = size / sizeof(uint32_t);
    const size_t computeSize = computeElementsCount * sizeof(float);
    const uint32_t computeIterations = static_cast<uint32_t>(arguments.computeIterations);

    ze_module_handle_t module{};
    ze_kernel_handle_t kernel{};
    if (auto result = L0::KernelHelper::loadKernel(levelzero, "graph_api_benchmark_kernel_add_one.cl", "add_one", &kernel, &module, nullptr);
        result != TestResult::Success) {
        return result;
    }

    uint32_t groupSizeX{}, groupSizeY{}, groupSizeZ{};
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSuggestGroupSize(kernel, static_cast<uint32_t>(elementsCount), 1u, 1u, &groupSizeX, &groupSizeY, &groupSizeZ));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, groupSizeX, groupSizeY, groupSizeZ));
    const ze_group_count_t groupCount{static_cast<uint32_t>(elementsCount / groupSizeX), 1u, 1u};

    ze_module_handle_t computeModule{};
    ze_kernel_handle_t computeKernelFirst{};
    ze_kernel_handle_t computeKernelNext{};
    ze_group_count_t computeGroupCount{};
    if (arguments.withComputeGraph) {
        if (auto result = L0::KernelHelper::loadKernel(levelzero, "graph_api_benchmark_kernel_compute_bound.cl", "compute_bound", &computeKernelFirst, &computeModule, nullptr);
            result != TestResult::Success) {
            return result;
        }
        ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
        kernelDesc.pKernelName = "compute_bound";
        ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(computeModule, &kernelDesc, &computeKernelNext));

        ASSERT_ZE_RESULT_SUCCESS(zeKernelSuggestGroupSize(computeKernelFirst, static_cast<uint32_t>(computeElementsCount), 1u, 1u, &groupSizeX, &groupSizeY, &groupSizeZ));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(computeKernelFirst, groupSizeX, groupSizeY, groupSizeZ));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(computeKernelNext, groupSizeX, groupSizeY, groupSizeZ));
        computeGroupCount = {static_cast<uint32_t>(computeElementsCount / groupSizeX), 1u, 1u};
    }

    void *hostSrc{};
    void *hostDst{};
    void *deviceBuffer{};
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocHost(levelzero.context, &zeDefaultGPUHostMemAllocDesc, size, 0, &hostSrc));
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocHost(levelzero.context, &zeDefaultGPUHostMemAllocDesc, size, 0, &hostDst));
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &zeDefaultGPUDeviceMemAllocDesc, size, 0, levelzero.device, &deviceBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(deviceBuffer), &deviceBuffer));

    auto src = static_cast<uint32_t *>(hostSrc);
    auto dst = static_cast<uint32_t *>(hostDst);
    std::iota(src, src + elementsCount, 0u);
    std::memset(hostDst, 0, size);

    void *computeInput{};
    void *computeWork{};
    std::vector<float> computeInputHost;
    if (arguments.withComputeGraph) {
        ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &zeDefaultGPUDeviceMemAllocDesc, computeSize, 0, levelzero.device, &computeInput));
        ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &zeDefaultGPUDeviceMemAllocDesc, computeSize, 0, levelzero.device, &computeWork));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(computeKernelFirst, 0, sizeof(computeInput), &computeInput));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(computeKernelFirst, 1, sizeof(computeWork), &computeWork));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(computeKernelFirst, 2, sizeof(computeIterations), &computeIterations));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(computeKernelNext, 0, sizeof(computeWork), &computeWork));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(computeKernelNext, 1, sizeof(computeWork), &computeWork));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(computeKernelNext, 2, sizeof(computeIterations), &computeIterations));

        computeInputHost.resize(computeElementsCount);
        for (size_t i = 0; i < computeElementsCount; i++) {
            computeInputHost[i] = static_cast<float>(i % 1024) * 0.001f;
        }
    }

    ze_command_queue_desc_t commandQueueDesc{ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC};
    commandQueueDesc.ordinal = levelzero.commandQueueDesc.ordinal;
    commandQueueDesc.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
    commandQueueDesc.flags = ZE_COMMAND_QUEUE_FLAG_IN_ORDER;
    ze_command_list_handle_t computeCmdList{};
    if (arguments.withComputeGraph) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc, &computeCmdList));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(computeCmdList, computeInput, computeInputHost.data(), computeSize, nullptr, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(computeCmdList, std::numeric_limits<uint64_t>::max()));
    }
    if (arguments.withCopyOffload) {
        commandQueueDesc.flags |= ZE_COMMAND_QUEUE_FLAG_COPY_OFFLOAD_HINT;
    }
    ze_command_list_handle_t cmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc, &cmdList));

    ze_graph_handle_t graph{};
    ze_executable_graph_handle_t execGraph{};
    ASSERT_ZE_RESULT_SUCCESS(levelzero.graphExtension.graphCreate(levelzero.context, &graph, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(levelzero.graphExtension.commandListBeginCaptureIntoGraph(cmdList, graph, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, deviceBuffer, hostSrc, size, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, hostDst, deviceBuffer, size, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(levelzero.graphExtension.commandListEndGraphCapture(cmdList, &graph, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(levelzero.graphExtension.commandListInstantiateGraph(graph, &execGraph, nullptr));

    ze_graph_handle_t computeGraph{};
    ze_executable_graph_handle_t computeExecGraph{};
    if (arguments.withComputeGraph) {
        ASSERT_ZE_RESULT_SUCCESS(levelzero.graphExtension.graphCreate(levelzero.context, &computeGraph, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(levelzero.graphExtension.commandListBeginCaptureIntoGraph(computeCmdList, computeGraph, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(computeCmdList, computeKernelFirst, &computeGroupCount, nullptr, 0, nullptr));
        for (auto kernelIndex = 1u; kernelIndex < computeKernelCount; kernelIndex++) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(computeCmdList, computeKernelNext, &computeGroupCount, nullptr, 0, nullptr));
        }
        ASSERT_ZE_RESULT_SUCCESS(levelzero.graphExtension.commandListEndGraphCapture(computeCmdList, &computeGraph, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(levelzero.graphExtension.commandListInstantiateGraph(computeGraph, &computeExecGraph, nullptr));
    }

    ASSERT_ZE_RESULT_SUCCESS(levelzero.graphExtension.commandListAppendGraph(cmdList, execGraph, nullptr, nullptr, 0, nullptr));
    if (arguments.withComputeGraph) {
        ASSERT_ZE_RESULT_SUCCESS(levelzero.graphExtension.commandListAppendGraph(computeCmdList, computeExecGraph, nullptr, nullptr, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(computeCmdList, std::numeric_limits<uint64_t>::max()));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, std::numeric_limits<uint64_t>::max()));
    std::memset(hostDst, 0, size);

    Timer timer;
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(levelzero.graphExtension.commandListAppendGraph(cmdList, execGraph, nullptr, nullptr, 0, nullptr));
        if (arguments.withComputeGraph) {
            ASSERT_ZE_RESULT_SUCCESS(levelzero.graphExtension.commandListAppendGraph(computeCmdList, computeExecGraph, nullptr, nullptr, 0, nullptr));
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(computeCmdList, std::numeric_limits<uint64_t>::max()));
        }
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, std::numeric_limits<uint64_t>::max()));
        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    auto testResult = TestResult::Success;
    for (size_t i = 0; i < elementsCount; i++) {
        if (dst[i] != src[i] + 1) {
            testResult = TestResult::VerificationFail;
            break;
        }
    }
    if (arguments.withComputeGraph) {
        std::vector<float> computeOutput(computeElementsCount);
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(computeCmdList, computeOutput.data(), computeWork, computeSize, nullptr, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(computeCmdList, std::numeric_limits<uint64_t>::max()));
        for (size_t i = 0; i < computeElementsCount; i += computeVerificationStride) {
            float expected = computeInputHost[i];
            for (auto kernelIndex = 0u; kernelIndex < computeKernelCount; kernelIndex++) {
                expected = computeBoundReference(expected, computeIterations);
            }
            if (computeOutput[i] != expected) {
                testResult = TestResult::VerificationFail;
                break;
            }
        }
    }

    if (arguments.withComputeGraph) {
        EXPECT_ZE_RESULT_SUCCESS(levelzero.graphExtension.executableGraphDestroy(computeExecGraph));
        EXPECT_ZE_RESULT_SUCCESS(levelzero.graphExtension.graphDestroy(computeGraph));
        EXPECT_ZE_RESULT_SUCCESS(zeCommandListDestroy(computeCmdList));
        EXPECT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, computeWork));
        EXPECT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, computeInput));
        EXPECT_ZE_RESULT_SUCCESS(zeKernelDestroy(computeKernelNext));
        EXPECT_ZE_RESULT_SUCCESS(zeKernelDestroy(computeKernelFirst));
        EXPECT_ZE_RESULT_SUCCESS(zeModuleDestroy(computeModule));
    }
    EXPECT_ZE_RESULT_SUCCESS(levelzero.graphExtension.executableGraphDestroy(execGraph));
    EXPECT_ZE_RESULT_SUCCESS(levelzero.graphExtension.graphDestroy(graph));
    EXPECT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    EXPECT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, deviceBuffer));
    EXPECT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, hostDst));
    EXPECT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, hostSrc));
    EXPECT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    EXPECT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));

    return testResult;
}

[[maybe_unused]] static RegisterTestCaseImplementation<GraphWithTransfers> registerTestCase(run, Api::L0);
