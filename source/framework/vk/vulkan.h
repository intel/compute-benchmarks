/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/test_case/test_case.h"
#include "framework/vk/utility/error.h"

#include <vulkan/vulkan.h>

namespace VK {

struct Vulkan {
    VkInstance instance{};
    VkPhysicalDevice physicalDevice{};
    VkPhysicalDeviceProperties physicalDeviceProperties{};
    VkDevice device{};
    VkQueue queue{};
    uint32_t queueFamilyIndex{};
    VkCommandPool commandPool{};

    Vulkan();
    Vulkan(const Vulkan &) = delete;
    Vulkan &operator=(const Vulkan &) = delete;
    ~Vulkan();

    VkShaderModule createShaderModule(const std::string &spirvPath);

    // Command buffers are freed together with the pool, so they need no RAII wrapper.
    VkCommandBuffer allocateCommandBuffer();
};

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
    VulkanShaderModule(Vulkan &vulkan, const std::string &spirvPath) : vulkan_(vulkan), shaderModule_(vulkan.createShaderModule(spirvPath)) {}
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

// Owns the pipeline layout as well. The workgroup size is baked in through specialization
// constant 0, so every workgroup size needs its own pipeline.
class VulkanComputePipeline : NoCopyOrMove {
  public:
    VulkanComputePipeline(Vulkan &vulkan, VkShaderModule shaderModule, uint32_t workgroupSize);
    ~VulkanComputePipeline() noexcept;
    operator VkPipeline() const { return pipeline_; }

  private:
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

} // namespace VK

using namespace VK;
