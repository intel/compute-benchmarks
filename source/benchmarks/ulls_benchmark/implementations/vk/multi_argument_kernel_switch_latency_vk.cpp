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

#include "definitions/multi_argument_kernel_switch_latency.h"

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

using namespace VK;

static TestResult run(const MultiArgumentKernelSwitchLatencyArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (arguments.profiling) {
        return TestResult::NoImplementation;
    }

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    Vulkan vulkan;
    Timer timer;

    const auto argumentCount = static_cast<uint32_t>(arguments.argumentCount);
    const auto pushConstantsSize = static_cast<uint32_t>(argumentCount * sizeof(VkDeviceAddress));
    if (!vulkan.supportsDispatch1D(arguments.workgroupSize, arguments.workgroupCount) ||
        !vulkan.bufferDeviceAddressSupported ||
        !vulkan.supportsPushConstantsSize(pushConstantsSize)) {
        return TestResult::DeviceNotCapable;
    }

    const std::vector<ShaderDefine> defines{{"ARG_COUNT", std::to_string(argumentCount)}};
    std::vector<uint32_t> spirv;
    if (const TestResult result = ShaderCompiler::compileComputeShaderToSpirv("ulls_benchmark_multi_argument_kernel_switch_latency.comp", defines, spirv);
        result != TestResult::Success) {
        return result;
    }
    const VulkanShaderModule shaderModule(vulkan, spirv);
    const VulkanComputePipeline pipeline(vulkan, shaderModule, static_cast<uint32_t>(arguments.workgroupSize), pushConstantsSize);

    constexpr VkDeviceSize argumentBufferSize = 4096u;
    constexpr VkBufferUsageFlags argumentBufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    std::vector<std::unique_ptr<VulkanBuffer>> allocations;
    std::vector<VkDeviceAddress> argumentAddresses;
    allocations.reserve(argumentCount);
    argumentAddresses.reserve(argumentCount);
    for (auto allocationId = 0u; allocationId < argumentCount; allocationId++) {
        allocations.push_back(std::make_unique<VulkanBuffer>(vulkan, argumentBufferSize, argumentBufferUsage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
        argumentAddresses.push_back(allocations.back()->deviceAddress());
    }

    const VkCommandBuffer commandBuffer = vulkan.allocateCommandBuffer();
    const VkCommandBufferBeginInfo commandBufferBeginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    ASSERT_VK_SUCCESS(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    for (auto kernelIndex = 0u; kernelIndex < arguments.kernelCount; kernelIndex++) {
        if (kernelIndex > 0) {
            recordInOrderExecutionBarrier(commandBuffer);
        }
        vkCmdPushConstants(commandBuffer, pipeline.layout(), VK_SHADER_STAGE_COMPUTE_BIT, 0u, pushConstantsSize, argumentAddresses.data());
        vkCmdDispatch(commandBuffer, static_cast<uint32_t>(arguments.workgroupCount), 1u, 1u);
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
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<MultiArgumentKernelSwitchLatency> registerTestCase(run, Api::Vulkan);
