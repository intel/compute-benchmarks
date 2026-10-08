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

#include "definitions/empty_kernel.h"

#include <gtest/gtest.h>
#include <vector>

static TestResult run(const EmptyKernelArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    Vulkan vulkan;
    Timer timer;

    const VkPhysicalDeviceLimits &limits = vulkan.physicalDeviceProperties.limits;
    if (arguments.workgroupSize > limits.maxComputeWorkGroupInvocations ||
        arguments.workgroupSize > limits.maxComputeWorkGroupSize[0] ||
        arguments.workgroupCount > limits.maxComputeWorkGroupCount[0]) {
        return TestResult::DeviceNotCapable;
    }

    std::vector<uint32_t> spirv;
    if (const TestResult result = ShaderCompiler::compileComputeShaderToSpirv("ulls_benchmark_empty_kernel.comp", {}, spirv);
        result != TestResult::Success) {
        return result;
    }
    VulkanShaderModule shaderModule(vulkan, spirv);
    VulkanComputePipeline pipeline(vulkan, shaderModule, static_cast<uint32_t>(arguments.workgroupSize), VK_NULL_HANDLE);

    VkCommandBuffer commandBuffer = vulkan.allocateCommandBuffer();
    VkCommandBufferBeginInfo commandBufferBeginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    ASSERT_VK_SUCCESS(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdDispatch(commandBuffer, static_cast<uint32_t>(arguments.workgroupCount), 1u, 1u);
    ASSERT_VK_SUCCESS(vkEndCommandBuffer(commandBuffer));

    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        ASSERT_VK_SUCCESS(vkQueueSubmit(vulkan.queue, 1, &submitInfo, VK_NULL_HANDLE));
        ASSERT_VK_SUCCESS(vkQueueWaitIdle(vulkan.queue));
        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<EmptyKernel> registerTestCase(run, Api::Vulkan);
