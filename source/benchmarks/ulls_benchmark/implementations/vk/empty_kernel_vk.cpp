/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"
#include "framework/vk/vulkan.h"

#include "definitions/empty_kernel.h"

#include <gtest/gtest.h>
#include <limits>

static TestResult run(const EmptyKernelArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    Vulkan vulkan;
    Timer timer;

    const VkPhysicalDeviceLimits &limits = vulkan.physicalDeviceProperties.limits;
    if (arguments.workgroupSize > limits.maxComputeWorkGroupInvocations ||
        arguments.workgroupSize > limits.maxComputeWorkGroupSize[0] ||
        arguments.workgroupCount > limits.maxComputeWorkGroupCount[0]) {
        return TestResult::DeviceNotCapable;
    }

    // Create pipeline
    const std::string spirvPath = "vk_ulls_benchmark_empty_kernel.spv";
    if (FileHelper::loadBinaryFile(spirvPath).empty()) {
        return TestResult::KernelNotFound;
    }
    VulkanShaderModule shaderModule(vulkan, spirvPath);
    VulkanComputePipeline pipeline(vulkan, shaderModule, static_cast<uint32_t>(arguments.workgroupSize));

    // Record the command buffer once, it is re-submitted in every iteration
    VkCommandBuffer commandBuffer = vulkan.allocateCommandBuffer();
    VkCommandBufferBeginInfo commandBufferBeginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    ASSERT_VK_SUCCESS(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdDispatch(commandBuffer, static_cast<uint32_t>(arguments.workgroupCount), 1u, 1u);
    ASSERT_VK_SUCCESS(vkEndCommandBuffer(commandBuffer));

    VulkanFence fence(vulkan);
    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        ASSERT_VK_SUCCESS(vkQueueSubmit(vulkan.queue, 1, &submitInfo, fence));
        ASSERT_VK_SUCCESS(vkWaitForFences(vulkan.device, 1, fence.address(), VK_TRUE, std::numeric_limits<uint64_t>::max()));
        timer.measureEnd();
        ASSERT_VK_SUCCESS(vkResetFences(vulkan.device, 1, fence.address()));
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<EmptyKernel> registerTestCase(run, Api::Vulkan);
