/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/vk/vulkan.h"

#include "framework/configuration.h"
#include "framework/utility/file_helper.h"

#include <cstring>
#include <limits>
#include <vector>

namespace VK {

Vulkan::Vulkan() {
    VkApplicationInfo applicationInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    applicationInfo.pApplicationName = "compute_benchmarks";
    applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.pEngineName = "compute_benchmarks";
    applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.apiVersion = VK_API_VERSION_1_2;

    const char *validationLayerName = "VK_LAYER_KHRONOS_validation";
    VkInstanceCreateInfo instanceCreateInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instanceCreateInfo.pApplicationInfo = &applicationInfo;
    if (Configuration::get().vkEnableValidation) {
        instanceCreateInfo.enabledLayerCount = 1;
        instanceCreateInfo.ppEnabledLayerNames = &validationLayerName;
    }
    VK_RESULT_SUCCESS_OR_ERROR(vkCreateInstance(&instanceCreateInfo, nullptr, &this->instance));

    uint32_t physicalDeviceCount = 0;
    VK_RESULT_SUCCESS_OR_ERROR(vkEnumeratePhysicalDevices(this->instance, &physicalDeviceCount, nullptr));
    if (physicalDeviceCount == 0) {
        FATAL_ERROR("No Vulkan devices found on the system.");
    }
    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    VK_RESULT_SUCCESS_OR_ERROR(vkEnumeratePhysicalDevices(this->instance, &physicalDeviceCount, physicalDevices.data()));

    const auto physicalDeviceIndex = static_cast<uint32_t>(Configuration::get().vkDeviceIndex);
    if (physicalDeviceIndex >= physicalDeviceCount) {
        FATAL_ERROR("Invalid Vulkan device index. deviceIndex=", physicalDeviceIndex, " deviceCount=", physicalDeviceCount);
    }
    this->physicalDevice = physicalDevices[physicalDeviceIndex];
    vkGetPhysicalDeviceProperties(this->physicalDevice, &this->physicalDeviceProperties);

    // Queue families have to be selected before the device is created, because Vulkan requests queues
    // at vkCreateDevice time and only retrieves their handles afterwards.
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(this->physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(this->physicalDevice, &queueFamilyCount, queueFamilies.data());

    constexpr uint32_t invalidQueueFamilyIndex = std::numeric_limits<uint32_t>::max();
    uint32_t computeOnlyFamily = invalidQueueFamilyIndex;
    uint32_t anyComputeFamily = invalidQueueFamilyIndex;
    for (uint32_t familyIndex = 0; familyIndex < queueFamilyCount; familyIndex++) {
        const VkQueueFlags flags = queueFamilies[familyIndex].queueFlags;
        if ((flags & VK_QUEUE_COMPUTE_BIT) == 0) {
            continue;
        }
        if (anyComputeFamily == invalidQueueFamilyIndex) {
            anyComputeFamily = familyIndex;
        }
        if ((flags & VK_QUEUE_GRAPHICS_BIT) == 0 && computeOnlyFamily == invalidQueueFamilyIndex) {
            computeOnlyFamily = familyIndex;
        }
    }
    if (computeOnlyFamily != invalidQueueFamilyIndex) {
        this->queueFamilyIndex = computeOnlyFamily;
    } else if (anyComputeFamily != invalidQueueFamilyIndex) {
        this->queueFamilyIndex = anyComputeFamily;
    } else {
        FATAL_ERROR("No compute capable queue family found on Vulkan device ", this->physicalDeviceProperties.deviceName);
    }

    const float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queueCreateInfo.queueFamilyIndex = this->queueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkDeviceCreateInfo deviceCreateInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    VK_RESULT_SUCCESS_OR_ERROR(vkCreateDevice(this->physicalDevice, &deviceCreateInfo, nullptr, &this->device));

    vkGetDeviceQueue(this->device, this->queueFamilyIndex, 0, &this->queue);

    VkCommandPoolCreateInfo commandPoolCreateInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCreateInfo.queueFamilyIndex = this->queueFamilyIndex;
    VK_RESULT_SUCCESS_OR_ERROR(vkCreateCommandPool(this->device, &commandPoolCreateInfo, nullptr, &this->commandPool));
}

Vulkan::~Vulkan() {
    if (this->device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(this->device);
        if (this->commandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(this->device, this->commandPool, nullptr);
        }
        vkDestroyDevice(this->device, nullptr);
    }
    if (this->instance != VK_NULL_HANDLE) {
        vkDestroyInstance(this->instance, nullptr);
    }
}

VkShaderModule Vulkan::createShaderModule(const std::string &spirvPath) {
    const auto spirvBinary = FileHelper::loadBinaryFile(spirvPath);
    if (spirvBinary.empty()) {
        FATAL_ERROR("Could not load SPIR-V binary from ", spirvPath);
    }
    if (spirvBinary.size() % sizeof(uint32_t) != 0) {
        FATAL_ERROR("SPIR-V binary size is not a multiple of 4, path=", spirvPath, " size=", spirvBinary.size());
    }

    // vkCreateShaderModule requires pCode to be 4-byte aligned, which std::vector<uint8_t> does not guarantee
    std::vector<uint32_t> spirvCode(spirvBinary.size() / sizeof(uint32_t));
    std::memcpy(spirvCode.data(), spirvBinary.data(), spirvBinary.size());

    VkShaderModuleCreateInfo shaderModuleCreateInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    shaderModuleCreateInfo.codeSize = spirvBinary.size();
    shaderModuleCreateInfo.pCode = spirvCode.data();

    VkShaderModule shaderModule{};
    VK_RESULT_SUCCESS_OR_ERROR(vkCreateShaderModule(this->device, &shaderModuleCreateInfo, nullptr, &shaderModule));
    return shaderModule;
}

VkCommandBuffer Vulkan::allocateCommandBuffer() {
    VkCommandBufferAllocateInfo allocateInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocateInfo.commandPool = this->commandPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer{};
    VK_RESULT_SUCCESS_OR_ERROR(vkAllocateCommandBuffers(this->device, &allocateInfo, &commandBuffer));
    return commandBuffer;
}

VulkanComputePipeline::VulkanComputePipeline(Vulkan &vulkan, VkShaderModule shaderModule, uint32_t workgroupSize) : vulkan_(vulkan) {
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipelineLayoutCreateInfo.setLayoutCount = 0;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    VK_RESULT_SUCCESS_OR_ERROR(vkCreatePipelineLayout(vulkan_.device, &pipelineLayoutCreateInfo, nullptr, &this->pipelineLayout_));

    const uint32_t localSizeX = workgroupSize;
    VkSpecializationMapEntry specializationMapEntry{};
    specializationMapEntry.constantID = 0;
    specializationMapEntry.offset = 0;
    specializationMapEntry.size = sizeof(uint32_t);

    VkSpecializationInfo specializationInfo{};
    specializationInfo.mapEntryCount = 1;
    specializationInfo.pMapEntries = &specializationMapEntry;
    specializationInfo.dataSize = sizeof(uint32_t);
    specializationInfo.pData = &localSizeX;

    VkPipelineShaderStageCreateInfo stageCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageCreateInfo.module = shaderModule;
    stageCreateInfo.pName = "main";
    stageCreateInfo.pSpecializationInfo = &specializationInfo;

    VkComputePipelineCreateInfo pipelineCreateInfo{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    pipelineCreateInfo.stage = stageCreateInfo;
    pipelineCreateInfo.layout = this->pipelineLayout_;
    VK_RESULT_SUCCESS_OR_ERROR(vkCreateComputePipelines(vulkan_.device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &this->pipeline_));
}

VulkanComputePipeline::~VulkanComputePipeline() noexcept {
    if (this->pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(vulkan_.device, this->pipeline_, nullptr);
    }
    if (this->pipelineLayout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(vulkan_.device, this->pipelineLayout_, nullptr);
    }
}

VulkanFence::VulkanFence(Vulkan &vulkan) : vulkan_(vulkan) {
    VkFenceCreateInfo fenceCreateInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    VK_RESULT_SUCCESS_OR_ERROR(vkCreateFence(vulkan_.device, &fenceCreateInfo, nullptr, &this->fence_));
}

VulkanFence::~VulkanFence() noexcept {
    if (this->fence_ != VK_NULL_HANDLE) {
        vkDestroyFence(vulkan_.device, this->fence_, nullptr);
    }
}

} // namespace VK
