/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/test_case/test_case.h"
#include "framework/vk/utility/error.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <vector>
#include <vulkan/vulkan.h>

namespace VK {

struct Vulkan {
    static constexpr uint32_t invalidMemoryTypeIndex = std::numeric_limits<uint32_t>::max();

    VkInstance instance{};
    VkPhysicalDevice physicalDevice{};
    VkPhysicalDeviceProperties physicalDeviceProperties{};
    VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties{};
    VkPhysicalDeviceFeatures physicalDeviceFeatures{};
    bool bufferDeviceAddressSupported = false;
    VkDevice device{};
    VkQueue queue{};
    uint32_t queueFamilyIndex{};
    VkCommandPool commandPool{};

    Vulkan();
    Vulkan(const Vulkan &) = delete;
    Vulkan &operator=(const Vulkan &) = delete;
    ~Vulkan();

    VkShaderModule createShaderModule(std::span<const uint32_t> spirv);

    // Command buffers are freed together with the pool, so they need no RAII wrapper.
    VkCommandBuffer allocateCommandBuffer();

    uint32_t findMemoryType(uint32_t memoryTypeBits, VkMemoryPropertyFlags requiredProperties) const;
    bool supportsDispatch1D(const size_t workgroupSize, const size_t workgroupCount) const;
    bool supportsStorageBuffersInOneSet(const size_t storageBufferCount) const;
    bool supportsStorageBufferArrayDynamicIndexing() const;
    bool supportsPushConstantsSize(const size_t pushConstantsSize) const;
};

void recordInOrderExecutionBarrier(const VkCommandBuffer commandBuffer);

void recordShaderWriteToHostBarrier(const VkCommandBuffer commandBuffer);

struct NoCopyOrMove {
    NoCopyOrMove() = default;
    ~NoCopyOrMove() = default;
    NoCopyOrMove(const NoCopyOrMove &) = delete;
    NoCopyOrMove &operator=(const NoCopyOrMove &) = delete;
    NoCopyOrMove(NoCopyOrMove &&) = delete;
    NoCopyOrMove &operator=(NoCopyOrMove &&) = delete;
};

class VulkanShaderModule : NoCopyOrMove {
  public:
    VulkanShaderModule(Vulkan &vulkan, std::span<const uint32_t> spirv) : vulkan_(vulkan), shaderModule_(vulkan.createShaderModule(spirv)) {}
    ~VulkanShaderModule() noexcept {
        if (shaderModule_ != VK_NULL_HANDLE) {
            vkDestroyShaderModule(vulkan_.device, shaderModule_, nullptr);
        }
    }
    operator VkShaderModule() const { return shaderModule_; }

  private:
    Vulkan &vulkan_;
    VkShaderModule shaderModule_ = VK_NULL_HANDLE;
};

class VulkanDescriptorSet : NoCopyOrMove {
  public:
    VulkanDescriptorSet(Vulkan &vulkan, uint32_t storageBufferCount, uint32_t setCount);
    ~VulkanDescriptorSet() noexcept;
    VkDescriptorSet operator[](uint32_t setIndex) const { return descriptorSets_[setIndex]; }
    const VkDescriptorSet *address(uint32_t setIndex) const { return &descriptorSets_[setIndex]; }
    VkDescriptorSetLayout layout() const { return layout_; }

    // The returned write points at bufferInfos, which must stay alive until it is passed to vkUpdateDescriptorSets.
    VkWriteDescriptorSet storageBufferWrite(const uint32_t setIndex, const uint32_t firstElement, const std::span<const VkDescriptorBufferInfo> bufferInfos) const;

    void writeWholeBuffers(const uint32_t setIndex, const std::span<const VkBuffer> buffers) const;
    void writeWholeBuffer(const uint32_t setIndex, const VkBuffer buffer) const { this->writeWholeBuffers(setIndex, {&buffer, 1}); }

  private:
    Vulkan &vulkan_;
    VkDescriptorSetLayout layout_ = VK_NULL_HANDLE;
    VkDescriptorPool pool_ = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptorSets_;
};

// Owns the pipeline layout as well. The workgroup size is baked in through specialization
// constant 0, so every workgroup size needs its own pipeline.
class VulkanComputePipeline : NoCopyOrMove {
  public:
    VulkanComputePipeline(Vulkan &vulkan, VkShaderModule shaderModule, uint32_t workgroupSize, VkDescriptorSetLayout descriptorSetLayout)
        : VulkanComputePipeline(vulkan, shaderModule, workgroupSize, descriptorSetLayout, 0u) {}
    VulkanComputePipeline(Vulkan &vulkan, VkShaderModule shaderModule, uint32_t workgroupSize, uint32_t pushConstantsSize)
        : VulkanComputePipeline(vulkan, shaderModule, workgroupSize, VK_NULL_HANDLE, pushConstantsSize) {}
    ~VulkanComputePipeline() noexcept;
    operator VkPipeline() const { return pipeline_; }
    VkPipelineLayout layout() const { return pipelineLayout_; }

  private:
    VulkanComputePipeline(Vulkan &vulkan, VkShaderModule shaderModule, uint32_t workgroupSize, VkDescriptorSetLayout descriptorSetLayout, uint32_t pushConstantsSize);

    Vulkan &vulkan_;
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
};

class VulkanFence : NoCopyOrMove {
  public:
    explicit VulkanFence(Vulkan &vulkan);
    ~VulkanFence() noexcept;
    operator VkFence() const { return fence_; }
    const VkFence *address() const { return &fence_; }

  private:
    Vulkan &vulkan_;
    VkFence fence_ = VK_NULL_HANDLE;
};

class VulkanBuffer : NoCopyOrMove {
  public:
    VulkanBuffer(Vulkan &vulkan, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties);
    ~VulkanBuffer() noexcept;
    operator VkBuffer() const { return buffer_; }
    VkDeviceAddress deviceAddress() const;
    void *mappedPtr() const { return mappedPtr_; }
    void flush() const;
    void invalidate() const;

  private:
    Vulkan &vulkan_;
    VkBuffer buffer_ = VK_NULL_HANDLE;
    VkDeviceMemory memory_ = VK_NULL_HANDLE;
    void *mappedPtr_ = nullptr;
    bool hostCoherent_ = false;
};

} // namespace VK
