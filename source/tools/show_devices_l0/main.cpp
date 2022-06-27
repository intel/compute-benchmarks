/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/intel_product/get_intel_product_l0.h"
#include "framework/l0/utility/error.h"

#include <cstring>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

int printAvailableEngines(ze_device_handle_t device, uint32_t numberOfTabs) {
    uint32_t numQueueGroups = 0;
    ze_result_t res = zeDeviceGetCommandQueueGroupProperties(device, &numQueueGroups, nullptr);
    if (numQueueGroups == 0) {
        std::cerr << "No queue groups found!\n";
        return -1;
    }
    std::vector<ze_command_queue_group_properties_t> queueProperties(numQueueGroups);
    zeDeviceGetCommandQueueGroupProperties(device, &numQueueGroups, queueProperties.data());

    std::string tabDelimiter = "";
    tabDelimiter.append(numberOfTabs, '\t');
    std::cout << tabDelimiter << "\t" << numQueueGroups << " queue groups found\n";

    for (uint32_t i = 0; i < numQueueGroups; i++) {
        if (queueProperties[i].flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) {
            std::cout << tabDelimiter << "\t Group " << i << " (compute): "
                      << queueProperties[i].numQueues << " queues\n";
        } else if ((queueProperties[i].flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) == 0 &&
                   (queueProperties[i].flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY)) {
            std::cout << tabDelimiter << "\t Group " << i << " (copy): "
                      << queueProperties[i].numQueues << " queues\n";
        }
    }
    return 0;
}

int printDeviceProperties(ze_device_handle_t device, uint32_t numberOfTabs) {
    ze_device_properties_t deviceProperties;
    ze_result_t res = zeDeviceGetProperties(device, &deviceProperties);
    if (res != ZE_RESULT_SUCCESS) {
        std::cerr << "zeDeviceGetProperties failed\n";
        return -1;
    }

    std::string tabDelimiter = "";
    tabDelimiter.append(numberOfTabs, '\t');

    if (numberOfTabs == 0) {
        std::cout << tabDelimiter << "Device properties\n";
    } else {
        std::cout << tabDelimiter << "Subdevice properties\n";
    }
    std::cout << tabDelimiter << "\tname:        " << deviceProperties.name << "\n"
              << tabDelimiter << "\tdeviceId:    0x"
              << std::setw(4) << std::hex << std::setfill('0')
              << deviceProperties.deviceId << "\n"
              << tabDelimiter << "\tsubdeviceId: " << deviceProperties.subdeviceId << "\n"
              << tabDelimiter << "\tUUID: ";
    for (uint32_t i = 0; i < ZE_MAX_DEVICE_UUID_SIZE; i++) {
        std::cout << static_cast<uint32_t>(deviceProperties.uuid.id[i]) << " ";
    }
    std::cout << "\n";

    return 0;
}

int printPropertiesForAllSubDevices(ze_device_handle_t device, uint32_t numberOfTabs) {
    int ret = printDeviceProperties(device, numberOfTabs);
    if (ret) {
        return -1;
    }

    ret = printAvailableEngines(device, numberOfTabs);
    if (ret) {
        return -1;
    }

    uint32_t subDeviceCount = 0;
    ze_result_t res = zeDeviceGetSubDevices(device, &subDeviceCount, nullptr);
    if (res) {
        std::cerr << "zeDeviceGetSubDevices failed\n";
        return -1;
    }
    if (subDeviceCount == 0) {
        return 0;
    }

    std::vector<ze_device_handle_t> subDevices(subDeviceCount);
    zeDeviceGetSubDevices(device, &subDeviceCount, subDevices.data());
    for (auto subDevice : subDevices) {
        ret = printPropertiesForAllSubDevices(subDevice, numberOfTabs + 1);
        if (ret) {
            return -1;
        }
    }
    return 0;
}

struct deviceAndProperty {
    ze_pci_ext_properties_t properties;
    uint32_t deviceIndex;
};

int printDevicesWithTheSameBdfAddress(std::vector<ze_device_handle_t> &devices) {
    if (devices.size() == 0u) {
        return 0u;
    }

    ze_pci_ext_properties_t pciProperties = {};
    auto counter = 0u;
    std::vector<deviceAndProperty> properties;
    properties.resize(devices.size());
    auto dataIdentifier = 0u;

    for (const auto &device : devices) {
        properties.at(dataIdentifier).properties.stype = ZE_STRUCTURE_TYPE_PCI_EXT_PROPERTIES;
        properties.at(dataIdentifier).deviceIndex = dataIdentifier;
        ze_result_t res = zeDevicePciGetPropertiesExt(device, &properties.at(dataIdentifier++).properties);

        if (res) {
            std::cerr << "zeDevicePciGetPropertiesExt failed\n";
            return -1;
        }
    }

    //locate devices under the same PCI BUS
    while (properties.size() > 0u) {
        auto busId = properties[0].properties.address.bus;
        auto count = 0u;

        std::vector<uint32_t> deviceIdentifiers;

        for (const auto &property : properties) {
            if (busId == property.properties.address.bus) {
                count++;
                deviceIdentifiers.push_back(property.deviceIndex);
            }
        }
        std::cout << " Following number of devices " << count << " have the same PCI bus identifier " << busId << " Devices: ";
        for (const auto &handle : deviceIdentifiers) {
            std::cout << "Device " << handle << " ";
        }
        std::cout << std::endl;
        auto iterator = properties.begin();
        while (iterator != properties.end()) {
            if (iterator->properties.address.bus == busId) {
                iterator = properties.erase(iterator);
            } else {
                iterator++;
            }
        }
        deviceIdentifiers.clear();
    }
    return 0u;
}

int main() {
    Configuration::loadDefaultConfiguration();

    ze_result_t res = zeInit(ZE_INIT_FLAG_GPU_ONLY);
    if (res != ZE_RESULT_SUCCESS) {
        std::cerr << "zeInit failed\n";
        return -1;
    }

    uint32_t driverCount = 0;
    res = zeDriverGet(&driverCount, nullptr);
    if (driverCount == 0 || res != ZE_RESULT_SUCCESS) {
        std::cerr << "zeDriverGet failed\n";
        return -1;
    }
    std::vector<ze_driver_handle_t> drivers(driverCount);
    zeDriverGet(&driverCount, drivers.data());

    for (auto driver : drivers) {
        uint32_t deviceCount = 0;
        res = zeDeviceGet(driver, &deviceCount, nullptr);
        if (deviceCount == 0 || res != ZE_RESULT_SUCCESS) {
            std::cout << "No devices found\n";
            continue;
        }
        std::vector<ze_device_handle_t> devices(deviceCount);
        zeDeviceGet(driver, &deviceCount, devices.data());

        for (auto device : devices) {
            int ret = printPropertiesForAllSubDevices(device, 0);
            if (ret) {
                return -1;
            }
        }
        printDevicesWithTheSameBdfAddress(devices);
    }

    return 0;
}
