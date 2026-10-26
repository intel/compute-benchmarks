/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"
#include "framework/vk/shader_compiler.h"
#include "framework/vk/vulkan.h"

#include "definitions/multi_kernel_execution.h"

#include <gtest/gtest.h>
#include <optional>
#include <vector>

using namespace VK;

static TestResult run(const MultiKernelExecutionArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, arguments.profiling ? MeasurementType::Gpu : MeasurementType::Cpu);

    if (arguments.inOrderOverOOO || arguments.useMCL) {
        return TestResult::ApiNotCapable;
    }

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    Vulkan vulkan;
    Timer timer;

    if (!vulkan.supportsDispatch1D(arguments.workgroupSize, arguments.workgroupCount) ||
        (arguments.profiling && !vulkan.supportsTimestamps())) {
        return TestResult::DeviceNotCapable;
    }

    std::vector<uint32_t> spirv;
    if (const TestResult result = ShaderCompiler::compileComputeShaderToSpirv("ulls_benchmark_empty_kernel.comp", {}, spirv);
        result != TestResult::Success) {
        return result;
    }
    const VulkanShaderModule shaderModule(vulkan, spirv);
    const VulkanComputePipeline pipeline(vulkan, shaderModule, static_cast<uint32_t>(arguments.workgroupSize), VK_NULL_HANDLE);

    std::optional<VulkanGpuTimer> gpuTimer;
    if (arguments.profiling) {
        gpuTimer.emplace(vulkan);
    }

    const VkCommandBuffer commandBuffer = vulkan.allocateCommandBuffer();
    const VkCommandBufferBeginInfo commandBufferBeginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    ASSERT_VK_SUCCESS(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));
    if (gpuTimer) {
        gpuTimer->recordBegin(commandBuffer);
    }
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    for (auto kernelIndex = 0u; kernelIndex < arguments.kernelCount; kernelIndex++) {
        if (kernelIndex > 0) {
            recordInOrderExecutionBarrier(commandBuffer);
        }
        vkCmdDispatch(commandBuffer, static_cast<uint32_t>(arguments.workgroupCount), 1u, 1u);
    }
    if (gpuTimer) {
        gpuTimer->recordEnd(commandBuffer);
    }
    ASSERT_VK_SUCCESS(vkEndCommandBuffer(commandBuffer));

    const VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
    };

    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        ASSERT_VK_SUCCESS(vkQueueSubmit(vulkan.queue, 1, &submitInfo, VK_NULL_HANDLE));
        ASSERT_VK_SUCCESS(vkQueueWaitIdle(vulkan.queue));
        timer.measureEnd();
        if (gpuTimer) {
            std::chrono::nanoseconds gpuTime{};
            ASSERT_VK_SUCCESS(gpuTimer->getElapsedTime(gpuTime));
            statistics.pushValue(gpuTime, typeSelector.getUnit(), typeSelector.getType());
        } else {
            statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
        }
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<MultiKernelExecution> registerTestCase(run, Api::Vulkan);
