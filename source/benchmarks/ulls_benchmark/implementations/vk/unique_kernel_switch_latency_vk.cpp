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

#include "definitions/unique_kernel_switch_latency.h"

#include <gtest/gtest.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>

using namespace VK;

static TestResult run(const UniqueKernelSwitchLatencyArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, arguments.profiling ? MeasurementType::Gpu : MeasurementType::Cpu);

    if (arguments.kernelCount < 2) {
        return TestResult::InvalidArgs;
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

    const VulkanDescriptorSet descriptorSets(vulkan, 1, 1);
    const auto kernelCount = static_cast<uint32_t>(arguments.kernelCount);
    std::vector<std::unique_ptr<VulkanShaderModule>> shaderModules;
    std::vector<std::unique_ptr<VulkanComputePipeline>> pipelines;
    shaderModules.reserve(kernelCount);
    pipelines.reserve(kernelCount);
    for (auto kernelIndex = 0u; kernelIndex < kernelCount; kernelIndex++) {
        const ShaderDefine defines[]{{"KERNEL_ID", std::to_string(kernelIndex)}};
        std::vector<uint32_t> spirv;
        if (const TestResult result = ShaderCompiler::compileComputeShaderToSpirv("ulls_benchmark_unique_kernel_switch_latency.comp", defines, spirv);
            result != TestResult::Success) {
            return result;
        }
        shaderModules.push_back(std::make_unique<VulkanShaderModule>(vulkan, spirv));
        pipelines.push_back(std::make_unique<VulkanComputePipeline>(vulkan, *shaderModules.back(), static_cast<uint32_t>(arguments.workgroupSize), descriptorSets.layout()));
    }

    constexpr VkDeviceSize argumentBufferSize = 4096u;
    const VulkanBuffer buffer(vulkan, argumentBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    descriptorSets.writeWholeBuffer(0, buffer);

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
    for (auto kernelIndex = 0u; kernelIndex < kernelCount; kernelIndex++) {
        if (kernelIndex > 0) {
            recordInOrderExecutionBarrier(commandBuffer);
        }
        const VulkanComputePipeline &pipeline = *pipelines[kernelIndex];
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.layout(), 0, 1, descriptorSets.address(0), 0, nullptr);
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

static RegisterTestCaseImplementation<UniqueKernelSwitchLatency> registerTestCase(run, Api::Vulkan);
