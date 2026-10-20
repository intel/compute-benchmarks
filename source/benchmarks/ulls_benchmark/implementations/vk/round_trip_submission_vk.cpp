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

#include "definitions/round_trip_submission.h"

#include <gtest/gtest.h>
#include <vector>

using namespace VK;

static TestResult run(const RoundTripSubmissionArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    Vulkan vulkan;
    Timer timer;

    constexpr VkDeviceSize bufferSize = 4096u;
    const VulkanBuffer buffer(vulkan, bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    std::vector<uint32_t> spirv;
    if (const TestResult result = ShaderCompiler::compileComputeShaderToSpirv("ulls_benchmark_write_one.comp", writeOneShaderDefines(WorkItemIdUsage::None), spirv);
        result != TestResult::Success) {
        return result;
    }
    constexpr uint32_t workgroupSize = 1u;
    constexpr uint32_t workgroupCount = 1u;
    const VulkanShaderModule shaderModule(vulkan, spirv);
    const VulkanDescriptorSet descriptorSets(vulkan, 1, 1);
    const VulkanComputePipeline pipeline(vulkan, shaderModule, workgroupSize, descriptorSets.layout());
    descriptorSets.writeWholeBuffer(0, buffer);

    const VkCommandBuffer commandBuffer = vulkan.allocateCommandBuffer();
    const VkCommandBufferBeginInfo commandBufferBeginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    ASSERT_VK_SUCCESS(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.layout(), 0, 1, descriptorSets.address(0), 0, nullptr);
    vkCmdDispatch(commandBuffer, workgroupCount, 1u, 1u);
    recordShaderWriteToHostBarrier(commandBuffer);
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
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<RoundTripSubmission> registerTestCase(run, Api::Vulkan);
