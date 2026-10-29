/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"
#include "framework/vk/vulkan.h"

#include "definitions/lifecycle_command_list.h"

using namespace VK;

static TestResult run(const LifecycleCommandListArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    Vulkan vulkan(arguments.copyOnly ? VulkanQueueType::CopyOnly : VulkanQueueType::Compute);
    if (vulkan.queue == VK_NULL_HANDLE) {
        return TestResult::DeviceNotCapable;
    }
    Timer timer;

    // Drivers can skip the submission of an empty command buffer, so each one carries a minimal fill.
    constexpr VkDeviceSize fillSize = sizeof(uint32_t);
    const VulkanBuffer fillBuffer(vulkan, fillSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    const VkCommandBufferAllocateInfo allocateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = vulkan.commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    const VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    VkCommandBuffer commandBuffer{};
    const VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
    };

    for (auto j = 0u; j < arguments.iterations; j++) {
        timer.measureStart();
        for (auto i = 0u; i < arguments.cmdListCount; i++) {
            ASSERT_VK_SUCCESS(vkAllocateCommandBuffers(vulkan.device, &allocateInfo, &commandBuffer));
            ASSERT_VK_SUCCESS(vkBeginCommandBuffer(commandBuffer, &beginInfo));
            vkCmdFillBuffer(commandBuffer, fillBuffer, 0, fillSize, 0u);
            ASSERT_VK_SUCCESS(vkEndCommandBuffer(commandBuffer));
            ASSERT_VK_SUCCESS(vkQueueSubmit(vulkan.queue, 1, &submitInfo, VK_NULL_HANDLE));
            ASSERT_VK_SUCCESS(vkQueueWaitIdle(vulkan.queue));
            vkFreeCommandBuffers(vulkan.device, vulkan.commandPool, 1, &commandBuffer);
        }
        timer.measureEnd();

        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }
    return TestResult::Success;
}

static RegisterTestCaseImplementation<LifecycleCommandList> registerTestCase(run, Api::Vulkan);
