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

#include "definitions/multi_argument_kernel.h"

#include <gtest/gtest.h>
#include <limits>
#include <memory>
#include <string>
#include <vector>

using namespace VK;

static TestResult run(const MultiArgumentKernelTimeArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (arguments.count > 1 && !arguments.measureSetKernelArg && !arguments.useL0NewArgApi) {
        return TestResult::NoImplementation;
    }

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    Vulkan vulkan;
    Timer timer;

    const auto argumentCount = static_cast<uint32_t>(arguments.argumentCount);
    if (!vulkan.supportsDispatch1D(arguments.lws, arguments.groupCount) ||
        !vulkan.supportsStorageBuffersInOneSet(argumentCount) ||
        !vulkan.supportsStorageBufferArrayDynamicIndexing()) {
        return TestResult::DeviceNotCapable;
    }

    const std::vector<ShaderDefine> defines{{"ARG_COUNT", std::to_string(argumentCount)},
                                            {"USE_GLOBAL_IDS", arguments.useGlobalIds ? "1" : "0"}};
    std::vector<uint32_t> spirv;
    if (const TestResult result = ShaderCompiler::compileComputeShaderToSpirv("api_overhead_benchmark_multi_arg_kernel.comp", defines, spirv);
        result != TestResult::Success) {
        return result;
    }
    VulkanShaderModule shaderModule(vulkan, spirv);
    const auto kernelCount = static_cast<uint32_t>(arguments.count);
    VulkanDescriptorSet descriptorSets(vulkan, argumentCount, kernelCount);
    VulkanComputePipeline pipeline(vulkan, shaderModule, static_cast<uint32_t>(arguments.lws), descriptorSets.layout());
    const VkPipelineLayout pipelineLayout = pipeline.layout();

    std::vector<std::unique_ptr<VulkanBuffer>> allocations;
    allocations.reserve(argumentCount);
    std::vector<VkDescriptorBufferInfo> bufferInfos(argumentCount);
    std::vector<VkDescriptorBufferInfo> reversedBufferInfos(argumentCount);
    for (auto allocationId = 0u; allocationId < argumentCount; allocationId++) {
        allocations.push_back(std::make_unique<VulkanBuffer>(vulkan, 4096u, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
        bufferInfos[allocationId] = VkDescriptorBufferInfo{*allocations.back(), 0u, VK_WHOLE_SIZE};
    }
    for (auto index = 0u; index < argumentCount; index++) {
        reversedBufferInfos[argumentCount - index - 1] = bufferInfos[index];
    }

    std::vector<VkWriteDescriptorSet> kernelArguments(kernelCount * argumentCount);
    std::vector<VkWriteDescriptorSet> reversedKernelArguments(kernelCount * argumentCount);
    for (auto kernelId = 0u; kernelId < kernelCount; kernelId++) {
        for (auto argumentId = 0u; argumentId < argumentCount; argumentId++) {
            const auto writeId = kernelId * argumentCount + argumentId;
            kernelArguments[writeId] = descriptorSets.storageBufferWrite(kernelId, argumentId, {&bufferInfos[argumentId], 1});
            reversedKernelArguments[writeId] = descriptorSets.storageBufferWrite(kernelId, argumentId, {&reversedBufferInfos[argumentId], 1});
        }
    }

    VkCommandBuffer commandBuffer = vulkan.allocateCommandBuffer();
    VulkanFence fence(vulkan);
    VkCommandBufferBeginInfo commandBufferBeginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    const auto groupCount = static_cast<uint32_t>(arguments.groupCount);
    bool reverseOrder = false;

    for (auto i = 0u; i < arguments.iterations; ++i) {
        ASSERT_VK_SUCCESS(vkResetCommandBuffer(commandBuffer, 0));
        if (arguments.reverseOrder) {
            reverseOrder = !reverseOrder;
        }
        const VkWriteDescriptorSet *writes = reverseOrder ? reversedKernelArguments.data() : kernelArguments.data();

        ASSERT_VK_SUCCESS(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

        if (arguments.useL0NewArgApi) {
            // No Vulkan counterpart to zeCommandListAppendLaunchKernelWithArguments exists, so the
            // closest analogue is used: all arguments of one kernel are written with a single
            // vkUpdateDescriptorSets call instead of one call per argument.
            timer.measureStart();
            for (auto kernelId = 0u; kernelId < kernelCount; ++kernelId) {
                vkUpdateDescriptorSets(vulkan.device, argumentCount, &writes[kernelId * argumentCount], 0, nullptr);
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, descriptorSets.address(kernelId), 0, nullptr);
                vkCmdDispatch(commandBuffer, groupCount, 1u, 1u);
            }
        } else {
            bool timerStarted = false;
            for (auto kernelId = 0u; kernelId < kernelCount; ++kernelId) {
                if (arguments.measureSetKernelArg && !timerStarted) {
                    timerStarted = true;
                    timer.measureStart();
                }

                for (auto argumentId = 0u; argumentId < argumentCount; ++argumentId) {
                    vkUpdateDescriptorSets(vulkan.device, 1, &writes[kernelId * argumentCount + argumentId], 0, nullptr);
                }

                if (!arguments.measureSetKernelArg && !timerStarted) {
                    timerStarted = true;
                    timer.measureStart();
                }
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, descriptorSets.address(kernelId), 0, nullptr);
                vkCmdDispatch(commandBuffer, groupCount, 1u, 1u);
            }
        }
        ASSERT_VK_SUCCESS(vkEndCommandBuffer(commandBuffer));

        if (arguments.exec) {
            ASSERT_VK_SUCCESS(vkQueueSubmit(vulkan.queue, 1, &submitInfo, fence));
            ASSERT_VK_SUCCESS(vkWaitForFences(vulkan.device, 1, fence.address(), VK_TRUE, std::numeric_limits<uint64_t>::max()));
        }
        timer.measureEnd();
        if (arguments.exec) {
            ASSERT_VK_SUCCESS(vkResetFences(vulkan.device, 1, fence.address()));
        }

        statistics.pushValue(timer.get() / arguments.count, typeSelector.getUnit(), typeSelector.getType());
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<MultiArgumentKernelTime> registerTestCase(run, Api::Vulkan);
