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

#include "definitions/kernel_with_work.h"

#include <cstring>
#include <gtest/gtest.h>
#include <memory>
#include <vector>

using namespace VK;

static TestResult run(const KernelWithWorkArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    Vulkan vulkan;
    Timer timer;

    if (!vulkan.supportsDispatch1D(arguments.workgroupSize, arguments.workgroupCount)) {
        return TestResult::DeviceNotCapable;
    }

    const bool atomicPerWorkgroup = arguments.usedIds == WorkItemIdUsage::AtomicPerWorkgroup;

    const auto elementCount = static_cast<VkDeviceSize>(arguments.workgroupCount) * arguments.workgroupSize;
    const VkDeviceSize bufferSize = atomicPerWorkgroup ? 2 * sizeof(uint32_t) : elementCount * sizeof(uint32_t);
    if (bufferSize > vulkan.physicalDeviceProperties.limits.maxStorageBufferRange) {
        return TestResult::DeviceNotCapable;
    }
    VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    if (atomicPerWorkgroup) {
        bufferUsage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }
    VulkanBuffer buffer(vulkan, bufferSize, bufferUsage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    std::vector<uint32_t> spirv;
    if (const TestResult result = ShaderCompiler::compileComputeShaderToSpirv("ulls_benchmark_write_one.comp",
                                                                              writeOneShaderDefines(arguments.usedIds), spirv);
        result != TestResult::Success) {
        return result;
    }
    const VulkanShaderModule shaderModule(vulkan, spirv);
    const VulkanDescriptorSet descriptorSets(vulkan, 1, 1);
    const VulkanComputePipeline pipeline(vulkan, shaderModule, static_cast<uint32_t>(arguments.workgroupSize), descriptorSets.layout());

    descriptorSets.writeWholeBuffer(0, buffer);

    const VkCommandBuffer commandBuffer = vulkan.allocateCommandBuffer();
    const VkCommandBufferBeginInfo commandBufferBeginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    ASSERT_VK_SUCCESS(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.layout(), 0, 1, descriptorSets.address(0), 0, nullptr);
    vkCmdDispatch(commandBuffer, static_cast<uint32_t>(arguments.workgroupCount), 1u, 1u);
    recordShaderWriteToHostBarrier(commandBuffer);
    ASSERT_VK_SUCCESS(vkEndCommandBuffer(commandBuffer));

    std::unique_ptr<VulkanBuffer> staging;
    VkCommandBuffer initializeCommandBuffer{};
    VkCommandBuffer readCommandBuffer{};
    if (atomicPerWorkgroup) {
        staging = std::make_unique<VulkanBuffer>(vulkan, 2 * sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

        initializeCommandBuffer = vulkan.allocateCommandBuffer();
        ASSERT_VK_SUCCESS(vkBeginCommandBuffer(initializeCommandBuffer, &commandBufferBeginInfo));
        const VkBufferCopy counterCopy{0, 0, 2 * sizeof(uint32_t)};
        vkCmdCopyBuffer(initializeCommandBuffer, *staging, buffer, 1, &counterCopy);
        VkMemoryBarrier initializeBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        initializeBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        initializeBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        vkCmdPipelineBarrier(initializeCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &initializeBarrier, 0, nullptr, 0, nullptr);
        ASSERT_VK_SUCCESS(vkEndCommandBuffer(initializeCommandBuffer));

        readCommandBuffer = vulkan.allocateCommandBuffer();
        ASSERT_VK_SUCCESS(vkBeginCommandBuffer(readCommandBuffer, &commandBufferBeginInfo));
        VkMemoryBarrier readBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        readBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        readBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        vkCmdPipelineBarrier(readCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &readBarrier, 0, nullptr, 0, nullptr);
        const VkBufferCopy resultCopy{0, 0, 2 * sizeof(uint32_t)};
        vkCmdCopyBuffer(readCommandBuffer, buffer, *staging, 1, &resultCopy);
        VkMemoryBarrier hostBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        hostBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        hostBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
        vkCmdPipelineBarrier(readCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &hostBarrier, 0, nullptr, 0, nullptr);
        ASSERT_VK_SUCCESS(vkEndCommandBuffer(readCommandBuffer));
    }

    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    for (auto i = 0u; i < arguments.iterations; i++) {
        if (atomicPerWorkgroup) {
            const uint32_t initialValues[2]{static_cast<uint32_t>(arguments.workgroupCount), 0u};
            std::memcpy(staging->mappedPtr(), initialValues, sizeof(initialValues));
            staging->flush();
            submitInfo.pCommandBuffers = &initializeCommandBuffer;
            ASSERT_VK_SUCCESS(vkQueueSubmit(vulkan.queue, 1, &submitInfo, VK_NULL_HANDLE));
            ASSERT_VK_SUCCESS(vkQueueWaitIdle(vulkan.queue));
            submitInfo.pCommandBuffers = &commandBuffer;
        }

        timer.measureStart();
        ASSERT_VK_SUCCESS(vkQueueSubmit(vulkan.queue, 1, &submitInfo, VK_NULL_HANDLE));
        ASSERT_VK_SUCCESS(vkQueueWaitIdle(vulkan.queue));
        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());

        if (atomicPerWorkgroup) {
            submitInfo.pCommandBuffers = &readCommandBuffer;
            ASSERT_VK_SUCCESS(vkQueueSubmit(vulkan.queue, 1, &submitInfo, VK_NULL_HANDLE));
            ASSERT_VK_SUCCESS(vkQueueWaitIdle(vulkan.queue));
            staging->invalidate();
            uint32_t returnedValue[2]{};
            std::memcpy(returnedValue, staging->mappedPtr(), sizeof(returnedValue));
            EXPECT_EQ(0u, returnedValue[0]);
            EXPECT_EQ(1337u, returnedValue[1]);
        }
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<KernelWithWork> registerTestCase(run, Api::Vulkan);
