/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/intel_product/get_intel_product_ocl.h"
#include "framework/ocl/utility/error.h"

#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// ------------------------------------------------------------------------- Helpers for creating/freeing platforms and devices

std::vector<cl_platform_id> getPlatforms() {
    cl_uint numPlatforms;
    cl_int retVal = clGetPlatformIDs(0, nullptr, &numPlatforms);
    if (retVal != CL_SUCCESS) {
        std::cout << "OpenCL platforms: NONE (clGetPlatformIDs returned " << oclErrorToString(retVal) << ")\n";
        return {};
    }
    std::vector<cl_platform_id> platforms{numPlatforms};
    CL_SUCCESS_OR_ERROR(clGetPlatformIDs(numPlatforms, platforms.data(), nullptr), "Querying platforms");
    return platforms;
}

std::vector<cl_device_id> getDevices(cl_platform_id platform) {
    cl_uint numDevices;
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, nullptr, &numDevices);
    if (numDevices == 0) {
        return {};
    }

    std::vector<cl_device_id> devices{numDevices};
    CL_SUCCESS_OR_ERROR(clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, numDevices, devices.data(), nullptr), "Querying devices");
    return devices;
}

std::vector<cl_device_id> createSubDevices(cl_device_id rootDevice) {
    cl_device_affinity_domain domain{};
    CL_SUCCESS_OR_ERROR(clGetDeviceInfo(rootDevice, CL_DEVICE_PARTITION_AFFINITY_DOMAIN, sizeof(domain), &domain, NULL), "Querying subdevices");
    if ((domain & CL_DEVICE_AFFINITY_DOMAIN_NEXT_PARTITIONABLE) == 0) {
        return {};
    }
    if ((domain & CL_DEVICE_AFFINITY_DOMAIN_NUMA) == 0) {
        return {};
    }

    const cl_device_partition_property properties[] = {CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN, CL_DEVICE_AFFINITY_DOMAIN_NUMA, 0};
    cl_uint numSubDevices{};
    CL_SUCCESS_OR_ERROR(clCreateSubDevices(rootDevice, properties, 0, nullptr, &numSubDevices), "Creating subdevices");

    std::vector<cl_device_id> result = {};
    result.resize(numSubDevices);
    CL_SUCCESS_OR_ERROR(clCreateSubDevices(rootDevice, properties, numSubDevices, result.data(), nullptr), "Creating subdevices");
    return result;
}

void freeSubDevices(std::vector<cl_device_id> &subDevices) {
    for (cl_device_id subDevice : subDevices) {
        CL_SUCCESS_OR_ERROR(clReleaseDevice(subDevice), "Releasing subdevice");
    }
}

// ------------------------------------------------------------------------- Printing functions

void showPlatform(size_t indentLevel, cl_platform_id platform, size_t platformIndex) {
    const std::string indent0(indentLevel + 0, '\t');
    char bufferString[4096];

    CL_SUCCESS_OR_ERROR(clGetPlatformInfo(platform, CL_PLATFORM_NAME, sizeof(bufferString), bufferString, nullptr), "Querying platform name");
    std::cout << indent0 << "Platform " << platformIndex << ": " << bufferString << '\n';
}

void showDevice(size_t indentLevel, cl_device_id device, const std::string &deviceLabel, size_t deviceIndex) {
    const std::string indent0(indentLevel + 0, '\t');
    const std::string indent1(indentLevel + 1, '\t');
    const std::string indent2(indentLevel + 2, '\t');
    char bufferString[4096];

    // Print device header
    CL_SUCCESS_OR_ERROR(clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(bufferString), bufferString, nullptr), "Querying device name");
    std::cout << indent0 << deviceLabel << " " << deviceIndex << ": " << bufferString;
    if (const IntelProduct intelProduct = getIntelProduct(device); intelProduct != IntelProduct::Unknown) {
        std::cout << " (" << std::to_string(intelProduct) << ")";
    }
    std::cout << '\n';

    // Print queue families support
    const bool queueuFamiliesSupported = ExtensionsHelper{device}.isCommandQueueFamiliesSupported();
    if (!queueuFamiliesSupported) {
        std::cout << indent1 << "Extension cl_intel_command_queue_families is NOT SUPPORTED";
        return;
    }

    // Print default queue
    size_t defaultFamilyIndex{};
    size_t defaultQueueIndex{};
    {
        cl_int retVal{};
        cl_context context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &retVal);
        CL_SUCCESS_OR_ERROR(retVal, "Context creation");
        cl_command_queue defaultQueue = clCreateCommandQueueWithProperties(context, device, nullptr, &retVal);
        CL_SUCCESS_OR_ERROR(retVal, "Queue creation");

        CL_SUCCESS_OR_ERROR(clGetCommandQueueInfo(defaultQueue, CL_QUEUE_FAMILY_INTEL, sizeof(defaultFamilyIndex), &defaultFamilyIndex, nullptr), "Querying family index");
        CL_SUCCESS_OR_ERROR(clGetCommandQueueInfo(defaultQueue, CL_QUEUE_INDEX_INTEL, sizeof(defaultQueueIndex), &defaultQueueIndex, nullptr), "Querying queue index");

        CL_SUCCESS_OR_ERROR(clReleaseCommandQueue(defaultQueue), "Releasing queue");
        CL_SUCCESS_OR_ERROR(clReleaseContext(context), "Releasing context");
    }
    std::cout << indent1 << "defaultFamilyIndex=" << defaultFamilyIndex << " defaultQueueIndex=" << defaultQueueIndex << '\n';

    // Print queue families
    size_t familyPropertiesSize{};
    CL_SUCCESS_OR_ERROR(clGetDeviceInfo(device, CL_DEVICE_QUEUE_FAMILY_PROPERTIES_INTEL, 0, nullptr, &familyPropertiesSize), "Querying families");
    const size_t numQueueFamilies = familyPropertiesSize / sizeof(cl_queue_family_properties_intel);
    auto queueFamilies = std::make_unique<cl_queue_family_properties_intel[]>(numQueueFamilies);
    CL_SUCCESS_OR_ERROR(clGetDeviceInfo(device, CL_DEVICE_QUEUE_FAMILY_PROPERTIES_INTEL, familyPropertiesSize, queueFamilies.get(), nullptr), "Querying families");
    std::cout << indent1 << "Queue families:\n";
    for (size_t familyIndex = 0u; familyIndex < numQueueFamilies; familyIndex++) {
        const cl_queue_family_properties_intel &queueFamily = queueFamilies[familyIndex];
        std::cout << indent2 << queueFamily.count << ' ' << queueFamily.name;
        if (familyIndex == defaultFamilyIndex) {
            std::cout << " (default)";
        }
        std::cout << '\n';
    }
}

void showDeviceAndItsSubDevices(size_t indentLevel, size_t deviceLevel, cl_device_id device, size_t deviceIndex) {
    const std::string deviceNames[] = {
        "RootDevice",
        "SubDevice",
        "SubSubDevice",
    };
    const std::string deviceLabel = deviceNames[deviceLevel];

    showDevice(indentLevel, device, deviceLabel, deviceIndex);

    std::vector<cl_device_id> subDevices = createSubDevices(device);
    for (auto subDeviceIndex = 0u; subDeviceIndex < subDevices.size(); subDeviceIndex++) {
        showDeviceAndItsSubDevices(indentLevel + 1, deviceLevel + 1, subDevices[subDeviceIndex], subDeviceIndex);
    }
    freeSubDevices(subDevices);
}

// ------------------------------------------------------------------------- Main procedure

int main() {
    Configuration::loadDefaultConfiguration();

    std::vector<cl_platform_id> platforms = getPlatforms();
    for (auto platformIndex = 0u; platformIndex < platforms.size(); platformIndex++) {
        cl_platform_id platform = platforms[platformIndex];
        showPlatform(0u, platform, platformIndex);

        std::vector<cl_device_id> devices = getDevices(platform);
        for (auto deviceIndex = 0u; deviceIndex < devices.size(); deviceIndex++) {
            cl_device_id device = devices[deviceIndex];
            showDeviceAndItsSubDevices(1u, 0u, device, deviceIndex);
        }
    }

    return 0;
}
