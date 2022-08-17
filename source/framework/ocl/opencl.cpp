/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "opencl.h"

namespace OCL {

Opencl::Opencl(const QueueProperties &queueProperties, const ContextProperties &contextProperties) {
    // Get Platform
    cl_uint numPlatforms;
    EXPECT_CL_SUCCESS(clGetPlatformIDs(0, nullptr, &numPlatforms));

    auto platforms = std::make_unique<cl_platform_id[]>(numPlatforms);
    EXPECT_CL_SUCCESS(clGetPlatformIDs(numPlatforms, platforms.get(), nullptr));

    auto platformIndex = Configuration::get().oclPlatformIndex;
    cl_uint numDevices;

    if (platformIndex == -1) {
        for (uint32_t localPlatformIndex = 0u; localPlatformIndex < numPlatforms; localPlatformIndex++) {
            if (clGetDeviceIDs(platforms[localPlatformIndex], CL_DEVICE_TYPE_ALL, 0, nullptr, &numDevices) == CL_SUCCESS) {
                platformIndex = localPlatformIndex;
            }
        }
    }

    if (platformIndex >= numPlatforms) {
        FATAL_ERROR("Invalid OCL platform index. platformIndex=", platformIndex, " numPlatforms=", numPlatforms);
    }

    this->platform = platforms[platformIndex];

    // Create root device
    EXPECT_CL_SUCCESS(clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, nullptr, &numDevices));
    const auto deviceIndex = Configuration::get().oclDeviceIndex;
    if (deviceIndex >= numDevices) {
        FATAL_ERROR("Invalid OCL device index. deviceIndex=", deviceIndex, " numDevices=", numDevices);
    }
    auto devices = std::make_unique<cl_device_id[]>(numDevices);
    EXPECT_CL_SUCCESS(clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, numDevices, devices.get(), nullptr));
    this->rootDevice = devices[deviceIndex];

    // Create sub devices if needed
    if (DeviceSelectionHelper::hasAnySubDevice(contextProperties.deviceSelection)) {
        this->createSubDevices(contextProperties.requireCreationSuccess);
        const auto requiredSubDevicesCount = DeviceSelectionHelper::getMaxSubDeviceIndex(contextProperties.deviceSelection) + 1;
        if (this->subDevices.size() < requiredSubDevicesCount) {
            return;
        }
    }

    // Set the default device
    if (DeviceSelectionHelper::hasSingleDevice(contextProperties.deviceSelection)) {
        this->device = getDevice(contextProperties.deviceSelection);
    }

    // Create context on the default device
    this->context = createContext(contextProperties);
    if (this->context == nullptr) {
        return;
    }

    // Create command queue on the default device
    this->commandQueue = createQueue(queueProperties);
}

Opencl::~Opencl() {
    for (auto &queueToRelease : commandQueues) {
        EXPECT_CL_SUCCESS(clReleaseCommandQueue(queueToRelease));
    }
    for (auto &contextToRelease : contexts) {
        EXPECT_CL_SUCCESS(clReleaseContext(contextToRelease));
    }
    for (auto &subDeviceToRelease : subDevices) {
        EXPECT_CL_SUCCESS(clReleaseDevice(subDeviceToRelease));
    }
}

cl_command_queue Opencl::createQueue(QueueProperties queueProperties) {
    if (!queueProperties.createQueue) {
        return nullptr;
    }
    const cl_device_id deviceForQueue = getDevice(queueProperties.deviceSelection);
    const size_t maxPropertiesCount = 7u;
    cl_int retVal{};
    cl_command_queue queue{};
    cl_queue_properties properties[maxPropertiesCount] = {};

    // Create queue
    if (queueProperties.fillQueueProperties(deviceForQueue, properties, maxPropertiesCount)) {
        queue = clCreateCommandQueueWithProperties(this->context, deviceForQueue, properties, &retVal);
    }

    if (queueProperties.requireCreationSuccess) {
        CL_SUCCESS_OR_ERROR(retVal, "Command queue creation failed");
    }

    if (queue) {
        this->commandQueues.push_back(queue);
    }
    return queue;
}

cl_context Opencl::createContext(const ContextProperties &contextProperties) {
    if (!contextProperties.createContext) {
        return nullptr;
    }

    std::vector<cl_device_id> devicesForContext = getDevices(contextProperties.deviceSelection, false);
    if (devicesForContext.size() == 0) {
        FATAL_ERROR_IF(contextProperties.requireCreationSuccess, "Failed getting devices for context");
        return nullptr;
    }

    cl_int retVal{};
    cl_context createdContext = clCreateContext(nullptr, static_cast<cl_uint>(devicesForContext.size()), devicesForContext.data(), nullptr, nullptr, &retVal);
    if (contextProperties.requireCreationSuccess) {
        CL_SUCCESS_OR_ERROR(retVal, "Context creation failed");
    }

    if (createdContext) {
        this->contexts.push_back(createdContext);
    }
    return createdContext;
}

cl_device_id Opencl::getDevice(DeviceSelection deviceSelection) {
    FATAL_ERROR_IF(DeviceSelectionHelper::hasHost(deviceSelection), "Cannot get cl_device_id for host");
    FATAL_ERROR_UNLESS(DeviceSelectionHelper::hasSingleDevice(deviceSelection), "Cannot get multiple devices");
    if (deviceSelection == DeviceSelection::Root) {
        return this->rootDevice;
    }

    const auto subDeviceIndex = DeviceSelectionHelper::getSubDeviceIndex(deviceSelection);
    FATAL_ERROR_UNLESS((subDeviceIndex < this->subDevices.size()), "Invalid subDevice index");
    return this->subDevices[subDeviceIndex];
}

std::vector<cl_device_id> Opencl::getDevices(DeviceSelection deviceSelection, bool requireSuccess) {
    FATAL_ERROR_IF(DeviceSelectionHelper::hasHost(deviceSelection), "Cannot get cl_device_id for host");
    std::vector<cl_device_id> result = {};

    // Add root device
    if ((deviceSelection & DeviceSelection::Root) == DeviceSelection::Root) {
        FATAL_ERROR_IF(this->rootDevice == nullptr, "Root device has not been created yet");
        result.push_back(this->rootDevice);
    }

    // Add subDevices
    for (DeviceSelection subDevice : DeviceSelectionHelper::subDevices) {
        if (DeviceSelectionHelper::hasDevice(deviceSelection, subDevice)) {
            const auto subDeviceIndex = DeviceSelectionHelper::getSubDeviceIndex(subDevice);
            FATAL_ERROR_IF(subDeviceIndex >= subDevices.size() && requireSuccess, "Invalid subDevice selected")
            result.push_back(subDevices[subDeviceIndex]);
        }
    }

    // Validate number of devices
    const auto expectedCount = DeviceSelectionHelper::getDevicesCount(deviceSelection);
    const auto actualCount = result.size();
    if (expectedCount != actualCount) {
        FATAL_ERROR_IF(expectedCount != actualCount && requireSuccess, "Invalid number of devices accumulated");
        return {};
    }

    return result;
}

const ExtensionsHelper &Opencl::getExtensions() {
    if (extensionsHelper == nullptr) {
        extensionsHelper = std::make_unique<ExtensionsHelper>(this->rootDevice);
    }
    return *extensionsHelper;
}

bool Opencl::createSubDevices(bool requireSuccess) {
    if (subDevices.size() != 0) {
        return true;
    }

    cl_device_affinity_domain domain{};
    EXPECT_CL_SUCCESS(clGetDeviceInfo(this->rootDevice, CL_DEVICE_PARTITION_AFFINITY_DOMAIN, sizeof(domain), &domain, NULL));
    if ((domain & CL_DEVICE_AFFINITY_DOMAIN_NEXT_PARTITIONABLE) == 0) {
        FATAL_ERROR_IF(requireSuccess, "SubDevice was selected, but device is not partitionable");
        return false;
    }
    if ((domain & CL_DEVICE_AFFINITY_DOMAIN_NUMA) == 0) {
        FATAL_ERROR_IF(requireSuccess, "SubDevice was selected, but device is not CL_DEVICE_AFFINITY_DOMAIN_NUMA");
        return false;
    }

    const cl_device_partition_property properties[] = {CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN, CL_DEVICE_AFFINITY_DOMAIN_NUMA, 0};
    cl_uint numSubDevices{};
    EXPECT_CL_SUCCESS(clCreateSubDevices(this->rootDevice, properties, 0, nullptr, &numSubDevices));
    this->subDevices.resize(numSubDevices);
    EXPECT_CL_SUCCESS(clCreateSubDevices(this->rootDevice, properties, numSubDevices, this->subDevices.data(), nullptr));
    return true;
}

} // namespace OCL
