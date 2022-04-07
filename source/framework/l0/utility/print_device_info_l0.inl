/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/intel_product/get_intel_product_l0.h"
#include "framework/l0/levelzero.h"

#include <iomanip>

namespace L0 {

void printDeviceInfo() {
    ContextProperties contextProperties = ContextProperties::create().disable();
    QueueProperties queueProperties = QueueProperties::create().disable();
    LevelZero levelzero(queueProperties, contextProperties);

    ze_driver_properties_t driverProperties{ZE_STRUCTURE_TYPE_DRIVER_PROPERTIES};
    ZE_RESULT_SUCCESS_OR_ERROR(zeDriverGetProperties(levelzero.driver, &driverProperties));
    // Version recorded by L0 GPU driver in following format:
    // 31 - 24: Major
    // 23 - 16: Minor
    // 15 - 0: Build
    std::string driverVersion = std::to_string((driverProperties.driverVersion & 0xFF000000) >> 24) + "." +
                                std::to_string((driverProperties.driverVersion & 0x00FF0000) >> 16) + "." +
                                std::to_string(driverProperties.driverVersion & 0x0000FFFF);
    std::cout << "LevelZero driver version: " << driverVersion << std::endl;

    ze_device_properties_t deviceProperties{ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES};
    ZE_RESULT_SUCCESS_OR_ERROR(zeDeviceGetProperties(levelzero.device, &deviceProperties));
    IntelProduct intelProduct = getIntelProduct(deviceProperties);
    IntelGen intelGen = getIntelGen(intelProduct);
    std::cout << "\tDevice: " << deviceProperties.name << std::endl;
    std::cout << "\t\tvendorId:     0x" << std::hex << deviceProperties.vendorId << std::endl;
    std::cout << "\t\tdeviceId:     0x" << std::hex << deviceProperties.deviceId << " (intelProduct=" << std::to_string(intelProduct) << ", intelGen=" << std::to_string(intelGen) << ")\n";
    std::cout << "\t\tclockFreq:    " << std::dec << deviceProperties.coreClockRate << std::endl;
    std::cout << "\t\tconfig:       " << deviceProperties.numSlices << "x" << deviceProperties.numSubslicesPerSlice << "x" << deviceProperties.numEUsPerSubslice << std::endl;
    std::cout << "\t\teuCount:      " << deviceProperties.numSlices * deviceProperties.numSubslicesPerSlice * deviceProperties.numEUsPerSubslice << std::endl;
    std::cout << "\t\tthreadsPerEu: " << deviceProperties.numThreadsPerEU << std::endl;

    std::cout << std::endl;
}

static void printAvailableDevices() {
    ZE_RESULT_SUCCESS_OR_ERROR(zeInit(ZE_INIT_FLAG_GPU_ONLY));

    // Get drivers count
    uint32_t driverCount = 0;
    ZE_RESULT_SUCCESS_OR_ERROR(zeDriverGet(&driverCount, nullptr));
    if (driverCount == 0) {
        std::cout << "LevelZero drivers: NONE";
        return;
    }
    std::cout << "LevelZero drivers: " << driverCount << '\n';

    // Iterate over drivers
    auto drivers = std::make_unique<ze_driver_handle_t[]>(driverCount);
    ZE_RESULT_SUCCESS_OR_ERROR(zeDriverGet(&driverCount, drivers.get()));
    for (uint32_t driverIndex = 0; driverIndex < driverCount; driverIndex++) {
        // Print info about current driver
        ze_driver_handle_t driver = drivers[driverIndex];
        ze_driver_properties_t driverProperties{};
        ZE_RESULT_SUCCESS_OR_ERROR(zeDriverGetProperties(driver, &driverProperties));
        std::cout << "  Driver " << driverIndex << ": version 0x" << std::hex << driverProperties.driverVersion << std::dec << " ";
        uint32_t deviceCount = 0;
        ZE_RESULT_SUCCESS_OR_ERROR(zeDeviceGet(driver, &deviceCount, nullptr));
        if (deviceCount == 0) {
            std::cout << "(no GPU devices found)\n";
            continue;
        } else {
            std::cout << "(" << deviceCount << " GPU devices)\n";
        }

        // Iterate over devices
        auto devices = std::make_unique<ze_device_handle_t[]>(deviceCount);
        ZE_RESULT_SUCCESS_OR_ERROR(zeDeviceGet(driver, &deviceCount, devices.get()));
        for (uint32_t deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++) {
            ze_device_handle_t device = devices[deviceIndex];

            ze_device_properties_t deviceProperties{};
            ZE_RESULT_SUCCESS_OR_ERROR(zeDeviceGetProperties(device, &deviceProperties));
            std::cout << "    Device: " << deviceProperties.name << " ";

            IntelProduct intelProduct = getIntelProduct(device);
            if (intelProduct != IntelProduct::Unknown) {
                std::cout << "(" << std::to_string(intelProduct) << ") ";
            }

            std::cout << ", select this device with --l0DriverIndex=" << driverIndex << " --l0DeviceIndex=" << deviceIndex;
            std::cout << std::endl;
        }
    }

    std::cout << std::endl;
}

} // namespace L0
