/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/vk/vulkan.h"

#include "framework/configuration.h"

#include <limits>
#include <vector>

namespace VK {

namespace {

constexpr uint32_t invalidQueueFamilyIndex = std::numeric_limits<uint32_t>::max();

uint32_t findComputeQueueFamily(const std::vector<VkQueueFamilyProperties> &queueFamilies) {
    uint32_t computeOnlyFamily = invalidQueueFamilyIndex;
    uint32_t anyComputeFamily = invalidQueueFamilyIndex;
    for (uint32_t familyIndex = 0; familyIndex < queueFamilies.size(); familyIndex++) {
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
    return computeOnlyFamily != invalidQueueFamilyIndex ? computeOnlyFamily : anyComputeFamily;
}

uint32_t findCopyOnlyQueueFamily(const std::vector<VkQueueFamilyProperties> &queueFamilies) {
    constexpr VkQueueFlags nonCopyEngineFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_VIDEO_DECODE_BIT_KHR | VK_QUEUE_VIDEO_ENCODE_BIT_KHR;
    for (uint32_t familyIndex = 0; familyIndex < queueFamilies.size(); familyIndex++) {
        const VkQueueFlags flags = queueFamilies[familyIndex].queueFlags;
        if ((flags & VK_QUEUE_TRANSFER_BIT) != 0 && (flags & nonCopyEngineFlags) == 0) {
            return familyIndex;
        }
    }
    return invalidQueueFamilyIndex;
}

} // namespace

Vulkan::Vulkan(const VulkanQueueType queueType) {
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
    vkGetPhysicalDeviceMemoryProperties(this->physicalDevice, &this->physicalDeviceMemoryProperties);
    vkGetPhysicalDeviceFeatures(this->physicalDevice, &this->physicalDeviceFeatures);
    if (this->physicalDeviceProperties.apiVersion >= VK_API_VERSION_1_2) {
        VkPhysicalDeviceBufferDeviceAddressFeatures supportedBufferDeviceAddressFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES};
        VkPhysicalDeviceFeatures2 supportedFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
        supportedFeatures.pNext = &supportedBufferDeviceAddressFeatures;
        vkGetPhysicalDeviceFeatures2(this->physicalDevice, &supportedFeatures);
        this->bufferDeviceAddressSupported = supportedBufferDeviceAddressFeatures.bufferDeviceAddress == VK_TRUE;
    }

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(this->physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(this->physicalDevice, &queueFamilyCount, queueFamilies.data());

    const uint32_t selectedQueueFamilyIndex = queueType == VulkanQueueType::CopyOnly ? findCopyOnlyQueueFamily(queueFamilies) : findComputeQueueFamily(queueFamilies);
    if (selectedQueueFamilyIndex == invalidQueueFamilyIndex) {
        if (queueType == VulkanQueueType::CopyOnly) {
            return;
        }
        FATAL_ERROR("No compute capable queue family found on Vulkan device ", this->physicalDeviceProperties.deviceName);
    }
    this->queueFamilyIndex = selectedQueueFamilyIndex;
    this->timestampValidBits = queueFamilies[this->queueFamilyIndex].timestampValidBits;

    const float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queueCreateInfo.queueFamilyIndex = this->queueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures enabledFeatures{};
    enabledFeatures.shaderStorageBufferArrayDynamicIndexing = this->physicalDeviceFeatures.shaderStorageBufferArrayDynamicIndexing;

    VkPhysicalDeviceBufferDeviceAddressFeatures enabledBufferDeviceAddressFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES};
    enabledBufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;

    VkDeviceCreateInfo deviceCreateInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    if (this->bufferDeviceAddressSupported) {
        deviceCreateInfo.pNext = &enabledBufferDeviceAddressFeatures;
    }
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.pEnabledFeatures = &enabledFeatures;
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

VkShaderModule Vulkan::createShaderModule(std::span<const uint32_t> spirv) {
    FATAL_ERROR_IF(spirv.empty(), "Cannot create a shader module from empty SPIR-V");

    VkShaderModuleCreateInfo shaderModuleCreateInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    shaderModuleCreateInfo.codeSize = spirv.size_bytes();
    shaderModuleCreateInfo.pCode = spirv.data();

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

uint32_t Vulkan::findMemoryType(uint32_t memoryTypeBits, VkMemoryPropertyFlags requiredProperties) const {
    for (uint32_t memoryTypeIndex = 0; memoryTypeIndex < this->physicalDeviceMemoryProperties.memoryTypeCount; memoryTypeIndex++) {
        if ((memoryTypeBits & (1u << memoryTypeIndex)) == 0) {
            continue;
        }
        const VkMemoryPropertyFlags flags = this->physicalDeviceMemoryProperties.memoryTypes[memoryTypeIndex].propertyFlags;
        if ((flags & requiredProperties) == requiredProperties) {
            return memoryTypeIndex;
        }
    }
    return invalidMemoryTypeIndex;
}

bool Vulkan::supportsDispatch1D(const size_t workgroupSize, const size_t workgroupCount) const {
    const VkPhysicalDeviceLimits &limits = this->physicalDeviceProperties.limits;
    return workgroupSize <= limits.maxComputeWorkGroupInvocations &&
           workgroupSize <= limits.maxComputeWorkGroupSize[0] &&
           workgroupCount <= limits.maxComputeWorkGroupCount[0];
}

bool Vulkan::supportsStorageBuffersInOneSet(const size_t storageBufferCount) const {
    const VkPhysicalDeviceLimits &limits = this->physicalDeviceProperties.limits;
    return storageBufferCount <= limits.maxPerStageDescriptorStorageBuffers &&
           storageBufferCount <= limits.maxDescriptorSetStorageBuffers;
}

bool Vulkan::supportsStorageBufferArrayDynamicIndexing() const {
    return this->physicalDeviceFeatures.shaderStorageBufferArrayDynamicIndexing == VK_TRUE;
}

bool Vulkan::supportsPushConstantsSize(const size_t pushConstantsSize) const {
    return pushConstantsSize <= this->physicalDeviceProperties.limits.maxPushConstantsSize;
}

void recordInOrderExecutionBarrier(const VkCommandBuffer commandBuffer) {
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0, nullptr);
}

void recordShaderWriteToHostBarrier(const VkCommandBuffer commandBuffer) {
    const VkMemoryBarrier shaderWriteBarrier{
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_HOST_READ_BIT,
    };
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &shaderWriteBarrier, 0, nullptr, 0, nullptr);
}

VulkanDescriptorSet::VulkanDescriptorSet(Vulkan &vulkan, uint32_t storageBufferCount, uint32_t setCount) : vulkan_(vulkan) {
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    binding.descriptorCount = storageBufferCount;
    binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layoutCreateInfo.bindingCount = 1;
    layoutCreateInfo.pBindings = &binding;
    VK_RESULT_SUCCESS_OR_ERROR(vkCreateDescriptorSetLayout(vulkan_.device, &layoutCreateInfo, nullptr, &this->layout_));

    const VkDescriptorPoolSize poolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, storageBufferCount * setCount};
    VkDescriptorPoolCreateInfo poolCreateInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    poolCreateInfo.maxSets = setCount;
    poolCreateInfo.poolSizeCount = 1;
    poolCreateInfo.pPoolSizes = &poolSize;
    VK_RESULT_SUCCESS_OR_ERROR(vkCreateDescriptorPool(vulkan_.device, &poolCreateInfo, nullptr, &this->pool_));

    const std::vector<VkDescriptorSetLayout> layouts(setCount, this->layout_);
    this->descriptorSets_.resize(setCount);
    VkDescriptorSetAllocateInfo allocateInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocateInfo.descriptorPool = this->pool_;
    allocateInfo.descriptorSetCount = setCount;
    allocateInfo.pSetLayouts = layouts.data();
    VK_RESULT_SUCCESS_OR_ERROR(vkAllocateDescriptorSets(vulkan_.device, &allocateInfo, this->descriptorSets_.data()));
}

VulkanDescriptorSet::~VulkanDescriptorSet() noexcept {
    if (this->pool_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(vulkan_.device, this->pool_, nullptr);
    }
    if (this->layout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(vulkan_.device, this->layout_, nullptr);
    }
}

VkWriteDescriptorSet VulkanDescriptorSet::storageBufferWrite(const uint32_t setIndex, const uint32_t firstElement, const std::span<const VkDescriptorBufferInfo> bufferInfos) const {
    return VkWriteDescriptorSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = this->descriptorSets_[setIndex],
        .dstBinding = 0,
        .dstArrayElement = firstElement,
        .descriptorCount = static_cast<uint32_t>(bufferInfos.size()),
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pBufferInfo = bufferInfos.data(),
    };
}

void VulkanDescriptorSet::writeWholeBuffers(const uint32_t setIndex, const std::span<const VkBuffer> buffers) const {
    std::vector<VkDescriptorBufferInfo> bufferInfos;
    bufferInfos.reserve(buffers.size());
    for (const VkBuffer buffer : buffers) {
        bufferInfos.push_back(VkDescriptorBufferInfo{buffer, 0u, VK_WHOLE_SIZE});
    }
    const VkWriteDescriptorSet write = this->storageBufferWrite(setIndex, 0u, bufferInfos);
    vkUpdateDescriptorSets(this->vulkan_.device, 1, &write, 0, nullptr);
}

VulkanComputePipeline::VulkanComputePipeline(Vulkan &vulkan, VkShaderModule shaderModule, uint32_t workgroupSize, VkDescriptorSetLayout descriptorSetLayout, uint32_t pushConstantsSize) : vulkan_(vulkan) {
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    if (descriptorSetLayout != VK_NULL_HANDLE) {
        pipelineLayoutCreateInfo.setLayoutCount = 1;
        pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
    }
    const VkPushConstantRange pushConstantRange{VK_SHADER_STAGE_COMPUTE_BIT, 0u, pushConstantsSize};
    if (pushConstantsSize > 0) {
        pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
        pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
    }
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

VulkanGpuTimer::VulkanGpuTimer(Vulkan &vulkan) : vulkan_(vulkan) {
    VkQueryPoolCreateInfo queryPoolCreateInfo{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
    queryPoolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    queryPoolCreateInfo.queryCount = 2;
    VK_RESULT_SUCCESS_OR_ERROR(vkCreateQueryPool(vulkan_.device, &queryPoolCreateInfo, nullptr, &this->queryPool_));
}

VulkanGpuTimer::~VulkanGpuTimer() noexcept {
    if (this->queryPool_ != VK_NULL_HANDLE) {
        vkDestroyQueryPool(vulkan_.device, this->queryPool_, nullptr);
    }
}

void VulkanGpuTimer::recordBegin(const VkCommandBuffer commandBuffer) const {
    vkCmdResetQueryPool(commandBuffer, this->queryPool_, 0, 2);
    vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, this->queryPool_, 0);
}

void VulkanGpuTimer::recordEnd(const VkCommandBuffer commandBuffer) const {
    vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, this->queryPool_, 1);
}

VkResult VulkanGpuTimer::getElapsedTime(std::chrono::nanoseconds &elapsedTime) const {
    uint64_t timestamps[2]{};
    const VkResult result = vkGetQueryPoolResults(vulkan_.device, this->queryPool_, 0, 2, sizeof(timestamps), timestamps, sizeof(uint64_t),
                                                  VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
    if (result != VK_SUCCESS) {
        return result;
    }

    const uint32_t validBits = vulkan_.timestampValidBits;
    const uint64_t validBitsMask = validBits >= 64 ? std::numeric_limits<uint64_t>::max() : (uint64_t{1} << validBits) - 1;
    const uint64_t elapsedTicks = (timestamps[1] - timestamps[0]) & validBitsMask;
    const std::chrono::duration<double, std::nano> elapsedTimeInNanoseconds(static_cast<double>(elapsedTicks) * vulkan_.physicalDeviceProperties.limits.timestampPeriod);
    elapsedTime = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsedTimeInNanoseconds);
    return VK_SUCCESS;
}

VulkanBuffer::VulkanBuffer(Vulkan &vulkan, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties) : vulkan_(vulkan) {
    VkBufferCreateInfo bufferCreateInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferCreateInfo.size = size;
    bufferCreateInfo.usage = usage;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_RESULT_SUCCESS_OR_ERROR(vkCreateBuffer(vulkan_.device, &bufferCreateInfo, nullptr, &this->buffer_));

    VkMemoryRequirements memoryRequirements{};
    vkGetBufferMemoryRequirements(vulkan_.device, this->buffer_, &memoryRequirements);
    const uint32_t memoryTypeIndex = vulkan_.findMemoryType(memoryRequirements.memoryTypeBits, memoryProperties);
    if (memoryTypeIndex == Vulkan::invalidMemoryTypeIndex) {
        FATAL_ERROR("No compatible memory type available for the requested buffer properties.");
    }

    VkMemoryAllocateFlagsInfo allocateFlagsInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO};
    allocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

    VkMemoryAllocateInfo allocateInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        allocateInfo.pNext = &allocateFlagsInfo;
    }
    allocateInfo.allocationSize = memoryRequirements.size;
    allocateInfo.memoryTypeIndex = memoryTypeIndex;
    VK_RESULT_SUCCESS_OR_ERROR(vkAllocateMemory(vulkan_.device, &allocateInfo, nullptr, &this->memory_));
    VK_RESULT_SUCCESS_OR_ERROR(vkBindBufferMemory(vulkan_.device, this->buffer_, this->memory_, 0));

    if (memoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        const VkMemoryPropertyFlags flags = vulkan_.physicalDeviceMemoryProperties.memoryTypes[memoryTypeIndex].propertyFlags;
        this->hostCoherent_ = (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0;
        VK_RESULT_SUCCESS_OR_ERROR(vkMapMemory(vulkan_.device, this->memory_, 0, VK_WHOLE_SIZE, 0, &this->mappedPtr_));
    }
}

VkDeviceAddress VulkanBuffer::deviceAddress() const {
    const VkBufferDeviceAddressInfo addressInfo{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, nullptr, this->buffer_};
    return vkGetBufferDeviceAddress(vulkan_.device, &addressInfo);
}

void VulkanBuffer::flush() const {
    if (this->mappedPtr_ == nullptr || this->hostCoherent_) {
        return;
    }
    VkMappedMemoryRange mappedRange{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
    mappedRange.memory = this->memory_;
    mappedRange.size = VK_WHOLE_SIZE;
    VK_RESULT_SUCCESS_OR_ERROR(vkFlushMappedMemoryRanges(vulkan_.device, 1, &mappedRange));
}

void VulkanBuffer::invalidate() const {
    if (this->mappedPtr_ == nullptr || this->hostCoherent_) {
        return;
    }
    VkMappedMemoryRange mappedRange{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
    mappedRange.memory = this->memory_;
    mappedRange.size = VK_WHOLE_SIZE;
    VK_RESULT_SUCCESS_OR_ERROR(vkInvalidateMappedMemoryRanges(vulkan_.device, 1, &mappedRange));
}

VulkanBuffer::~VulkanBuffer() noexcept {
    if (this->mappedPtr_ != nullptr) {
        vkUnmapMemory(vulkan_.device, this->memory_);
    }
    if (this->buffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(vulkan_.device, this->buffer_, nullptr);
    }
    if (this->memory_ != VK_NULL_HANDLE) {
        vkFreeMemory(vulkan_.device, this->memory_, nullptr);
    }
}

} // namespace VK
