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

#include "definitions/kernel_with_work.h"

#include <cstring>
#include <gtest/gtest.h>
#include <memory>

static TestResult run(const KernelWithWorkArguments &arguments, Statistics &statistics) {
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

    const bool atomicPerWorkgroup = arguments.usedIds == WorkItemIdUsage::AtomicPerWorkgroup;

    // Create output buffer
    const auto elementCount = static_cast<VkDeviceSize>(arguments.workgroupCount) * arguments.workgroupSize;
    const VkDeviceSize bufferSize = atomicPerWorkgroup ? 2 * sizeof(uint32_t) : elementCount * sizeof(uint32_t);
    if (bufferSize > limits.maxStorageBufferRange) {
        return TestResult::DeviceNotCapable;
    }
    VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    if (atomicPerWorkgroup) {
        bufferUsage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }
    VulkanBuffer buffer(vulkan, bufferSize, bufferUsage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // Create pipeline
    const std::string spirvPath = "vk_" + selectKernel(arguments.usedIds, "spv");
    if (FileHelper::loadBinaryFile(spirvPath).empty()) {
        return TestResult::KernelNotFound;
    }
    VulkanShaderModule shaderModule(vulkan, spirvPath);
    VulkanDescriptorSet descriptorSets(vulkan, 1);
    VulkanComputePipeline pipeline(vulkan, shaderModule, static_cast<uint32_t>(arguments.workgroupSize), descriptorSets.layout());

    const VkDescriptorBufferInfo bufferInfo{buffer, 0u, bufferSize};
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstSet = descriptorSets[0];
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.pBufferInfo = &bufferInfo;
    vkUpdateDescriptorSets(vulkan.device, 1, &write, 0, nullptr);

    // Record the command buffer
    VkCommandBuffer commandBuffer = vulkan.allocateCommandBuffer();
    VkCommandBufferBeginInfo commandBufferBeginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    ASSERT_VK_SUCCESS(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.layout(), 0, 1, descriptorSets.address(0), 0, nullptr);
    vkCmdDispatch(commandBuffer, static_cast<uint32_t>(arguments.workgroupCount), 1u, 1u);
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

    // Benchmark
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
