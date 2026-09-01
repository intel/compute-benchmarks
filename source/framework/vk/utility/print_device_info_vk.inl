/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/configuration.h"
#include "framework/vk/vulkan.h"

#include <vector>

namespace VK {

static const char *vkPhysicalDeviceTypeToString(VkPhysicalDeviceType deviceType) {
    switch (deviceType) {
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        return "INTEGRATED_GPU";
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        return "DISCRETE_GPU";
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
        return "VIRTUAL_GPU";
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
        return "CPU";
    default:
        return "OTHER";
    }
}

static std::string vkVersionToString(uint32_t version) {
    return std::to_string(VK_API_VERSION_MAJOR(version)) + "." +
           std::to_string(VK_API_VERSION_MINOR(version)) + "." +
           std::to_string(VK_API_VERSION_PATCH(version));
}

// Creates a throwaway instance instead of a full Vulkan object, so that device information can still
// be printed when vkDeviceIndex points at a non-existing device.
static VkInstance createInfoInstance() {
    VkApplicationInfo applicationInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    applicationInfo.pApplicationName = "compute_benchmarks";
    applicationInfo.pEngineName = "compute_benchmarks";
    applicationInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo instanceCreateInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instanceCreateInfo.pApplicationInfo = &applicationInfo;

    VkInstance instance{};
    VK_RESULT_SUCCESS_OR_ERROR(vkCreateInstance(&instanceCreateInfo, nullptr, &instance));
    return instance;
}

static std::vector<VkPhysicalDevice> getPhysicalDevices(VkInstance instance) {
    uint32_t physicalDeviceCount = 0;
    VK_RESULT_SUCCESS_OR_ERROR(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr));
    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    if (physicalDeviceCount > 0) {
        VK_RESULT_SUCCESS_OR_ERROR(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data()));
    }
    return physicalDevices;
}

static void printComputeQueueFamilies(std::ostream &output, VkPhysicalDevice physicalDevice) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    bool anyComputeFamily = false;
    for (uint32_t familyIndex = 0; familyIndex < queueFamilyCount; familyIndex++) {
        if ((queueFamilies[familyIndex].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0) {
            continue;
        }
        output << (anyComputeFamily ? ", " : "") << familyIndex << " (" << queueFamilies[familyIndex].queueCount << " queues)";
        anyComputeFamily = true;
    }
    if (!anyComputeFamily) {
        output << "NONE";
    }
}

void printDeviceInfo(std::ostream &output) {
    VkInstance instance = createInfoInstance();
    const auto physicalDevices = getPhysicalDevices(instance);
    const auto physicalDeviceIndex = static_cast<uint32_t>(Configuration::get().vkDeviceIndex);
    if (physicalDeviceIndex >= physicalDevices.size()) {
        output << "Vulkan device " << physicalDeviceIndex << " not found. Devices available: " << physicalDevices.size() << std::endl;
        vkDestroyInstance(instance, nullptr);
        return;
    }

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevices[physicalDeviceIndex], &properties);
    output << "\tDevice: " << properties.deviceName << std::endl;
    output << "\t\tapiVersion:    " << vkVersionToString(properties.apiVersion) << std::endl;
    output << "\t\tdriverVersion: 0x" << std::hex << properties.driverVersion << std::dec << std::endl;
    output << "\t\tdeviceType:    " << vkPhysicalDeviceTypeToString(properties.deviceType) << std::endl;
    output << "\t\tvendorId:      0x" << std::hex << properties.vendorID << std::dec << std::endl;
    output << "\t\tdeviceId:      0x" << std::hex << properties.deviceID << std::dec << std::endl;
    output << "\t\tcomputeQueueFamilies: ";
    printComputeQueueFamilies(output, physicalDevices[physicalDeviceIndex]);
    output << std::endl;

    output << std::endl;
    vkDestroyInstance(instance, nullptr);
}

static void printAvailableDevices() {
    VkInstance instance = createInfoInstance();
    const auto physicalDevices = getPhysicalDevices(instance);
    if (physicalDevices.empty()) {
        std::cout << "Vulkan devices: NONE" << std::endl;
        vkDestroyInstance(instance, nullptr);
        return;
    }
    std::cout << "Vulkan devices: " << physicalDevices.size() << '\n';

    for (size_t deviceIndex = 0; deviceIndex < physicalDevices.size(); deviceIndex++) {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physicalDevices[deviceIndex], &properties);
        std::cout << "  Device: " << properties.deviceName
                  << " (" << vkPhysicalDeviceTypeToString(properties.deviceType)
                  << ", apiVersion " << vkVersionToString(properties.apiVersion) << ") ";
        std::cout << "computeQueueFamilies: ";
        printComputeQueueFamilies(std::cout, physicalDevices[deviceIndex]);
        std::cout << ", select this device with --vkDeviceIndex=" << deviceIndex;
        std::cout << std::endl;
    }

    std::cout << std::endl;
    vkDestroyInstance(instance, nullptr);
}

} // namespace VK
