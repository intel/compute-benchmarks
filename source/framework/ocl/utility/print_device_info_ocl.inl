/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/intel_product/get_intel_product_ocl.h"
#include "framework/ocl/opencl.h"
#include "framework/ocl/utility/error_codes.h"

namespace OCL {

static void printDeviceInfo() {
    ContextProperties contextProperties = ContextProperties::create().disable();
    QueueProperties queueProperties = QueueProperties::create().disable();
    Opencl opencl(queueProperties, contextProperties);
    char bufferString[4096];
    cl_uint bufferUint;

    CL_SUCCESS_OR_ERROR(clGetPlatformInfo(opencl.platform, CL_PLATFORM_NAME, sizeof(bufferString), bufferString, nullptr), "clGetPlatformInfo failed");
    std::cout << "OpenCL Platform: " << bufferString << std::endl;

    CL_SUCCESS_OR_ERROR(clGetDeviceInfo(opencl.device, CL_DEVICE_NAME, sizeof(bufferString), bufferString, nullptr), "clGetDeviceInfo failed");
    std::cout << "\tDevice: " << bufferString << std::endl;

    CL_SUCCESS_OR_ERROR(clGetDeviceInfo(opencl.device, CL_DRIVER_VERSION, sizeof(bufferString), bufferString, nullptr), "clGetDeviceInfo failed");
    std::cout << "\t\tdriverVersion: " << bufferString << std::endl;

    CL_SUCCESS_OR_ERROR(clGetDeviceInfo(opencl.device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(bufferUint), &bufferUint, nullptr), "clGetDeviceInfo failed");
    std::cout << "\t\tcomputeUnits:  " << bufferUint << std::endl;

    CL_SUCCESS_OR_ERROR(clGetDeviceInfo(opencl.device, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(bufferUint), &bufferUint, nullptr), "clGetDeviceInfo failed");
    std::cout << "\t\tclockFreq:     " << bufferUint << std::endl;

    IntelProduct intelProduct = getIntelProduct(opencl);
    IntelGen intelGen = getIntelGen(intelProduct);
    if (intelProduct != IntelProduct::Unknown) {
        std::cout << "\t\tintelProduct:  " << std::to_string(intelProduct) << " (intelGen: " << std::to_string(intelGen) << ")\n";
    }

    std::cout << std::endl;
}

static void printAvailableDevices() {
    char bufferString[4096];

    // Get platforms count
    cl_uint numPlatforms;
    cl_int retVal = clGetPlatformIDs(0, nullptr, &numPlatforms);
    if (retVal != CL_SUCCESS) {
        std::cout << "OpenCL platforms: NONE (clGetPlatformIDs returned " << oclErrorToString(retVal) << ")\n";
        return;
    }
    std::cout << "OpenCL platforms: " << numPlatforms << '\n';

    // Iterate over platforms
    auto platforms = std::make_unique<cl_platform_id[]>(numPlatforms);
    EXPECT_CL_SUCCESS(clGetPlatformIDs(numPlatforms, platforms.get(), nullptr));
    for (cl_uint platformIndex = 0; platformIndex < numPlatforms; platformIndex++) {
        // Print info about current platform
        cl_platform_id platform = platforms[platformIndex];
        EXPECT_CL_SUCCESS(clGetPlatformInfo(platform, CL_PLATFORM_NAME, sizeof(bufferString), bufferString, nullptr));
        std::cout << "  Platform: " << bufferString << " ";
        cl_uint numDevices;
        retVal = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, nullptr, &numDevices);
        if (retVal != CL_SUCCESS) {
            std::cout << "(no GPU devices found, clGetDeviceIDs returned " << oclErrorToString(retVal) << ")\n";
            continue;
        } else {
            std::cout << "(" << numDevices << " GPU devices)\n";
        }

        // Iterate over devices
        auto devices = std::make_unique<cl_device_id[]>(numDevices);
        EXPECT_CL_SUCCESS(clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, numDevices, devices.get(), nullptr));
        for (cl_uint deviceIndex = 0; deviceIndex < numDevices; deviceIndex++) {
            cl_device_id device = devices[deviceIndex];

            EXPECT_CL_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(bufferString), bufferString, nullptr));
            std::cout << "    Device: " << bufferString << " ";

            IntelProduct intelProduct = getIntelProduct(device);
            if (intelProduct != IntelProduct::Unknown) {
                std::cout << "(" << std::to_string(intelProduct) << ") ";
            }

            std::cout << ", select this device with --oclPlatformIndex=" << platformIndex << " --oclDeviceIndex=" << deviceIndex;
            std::cout << std::endl;
        }
    }

    std::cout << std::endl;
}

} // namespace OCL
