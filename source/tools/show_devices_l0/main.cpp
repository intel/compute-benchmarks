/*
 * Copyright (C) 2022-2026 Intel Corporation
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
    EXPECT_ZE_RESULT_SUCCESS(zeDeviceGetCommandQueueGroupProperties(device, &numQueueGroups, queueProperties.data()));

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

std::string parseCaps(ze_memory_access_cap_flags_t flags) {
    std::string capsToReturn = "";
    if (flags & ZE_MEMORY_ACCESS_CAP_FLAG_RW) {
        capsToReturn += "ZE_MEMORY_ACCESS_CAP_FLAG_RW ";
    }
    if (flags & ZE_MEMORY_ACCESS_CAP_FLAG_ATOMIC) {
        capsToReturn += "ZE_MEMORY_ACCESS_CAP_FLAG_ATOMIC ";
    }
    if (flags & ZE_MEMORY_ACCESS_CAP_FLAG_CONCURRENT) {
        capsToReturn += "ZE_MEMORY_ACCESS_CAP_FLAG_CONCURRENT ";
    }
    if (flags & ZE_MEMORY_ACCESS_CAP_FLAG_CONCURRENT_ATOMIC) {
        capsToReturn += "ZE_MEMORY_ACCESS_CAP_FLAG_CONCURRENT_ATOMIC ";
    }

    if (flags == 0u) {
        capsToReturn += "UNSUPPORTED ";
    }

    return capsToReturn;
}

std::string parseFpFlags(ze_device_fp_flags_t flags) {
    if (flags == 0u)
        return "NONE";
    std::string s;
    if (flags & ZE_DEVICE_FP_FLAG_DENORM)
        s += "DENORM ";
    if (flags & ZE_DEVICE_FP_FLAG_INF_NAN)
        s += "INF_NAN ";
    if (flags & ZE_DEVICE_FP_FLAG_ROUND_TO_NEAREST)
        s += "ROUND_TO_NEAREST ";
    if (flags & ZE_DEVICE_FP_FLAG_ROUND_TO_ZERO)
        s += "ROUND_TO_ZERO ";
    if (flags & ZE_DEVICE_FP_FLAG_ROUND_TO_INF)
        s += "ROUND_TO_INF ";
    if (flags & ZE_DEVICE_FP_FLAG_FMA)
        s += "FMA ";
    if (flags & ZE_DEVICE_FP_FLAG_ROUNDED_DIVIDE_SQRT)
        s += "ROUNDED_DIVIDE_SQRT ";
    if (flags & ZE_DEVICE_FP_FLAG_SOFT_FLOAT)
        s += "SOFT_FLOAT ";
    return s;
}

int printComputeProperties(ze_device_handle_t device, uint32_t numberOfTabs) {
    ze_device_compute_properties_t computeProps = {ZE_STRUCTURE_TYPE_DEVICE_COMPUTE_PROPERTIES};
    if (zeDeviceGetComputeProperties(device, &computeProps) != ZE_RESULT_SUCCESS) {
        std::cerr << "zeDeviceGetComputeProperties failed\n";
        return -1;
    }
    std::string tab(numberOfTabs, '\t');
    std::cout << tab << "Compute properties:\n"
              << tab << "\tmaxTotalGroupSize:   " << computeProps.maxTotalGroupSize << "\n"
              << tab << "\tmaxGroupSize:        " << computeProps.maxGroupSizeX << " x "
              << computeProps.maxGroupSizeY << " x " << computeProps.maxGroupSizeZ << "\n"
              << tab << "\tmaxGroupCount:       " << computeProps.maxGroupCountX << " x "
              << computeProps.maxGroupCountY << " x " << computeProps.maxGroupCountZ << "\n"
              << tab << "\tmaxSharedLocalMemory:" << computeProps.maxSharedLocalMemory << "\n"
              << tab << "\tsubGroupSizes:       ";
    for (uint32_t i = 0; i < computeProps.numSubGroupSizes; i++) {
        std::cout << computeProps.subGroupSizes[i];
        if (i + 1 < computeProps.numSubGroupSizes)
            std::cout << ", ";
    }
    std::cout << "\n";
    return 0;
}

int printModuleProperties(ze_device_handle_t device, uint32_t numberOfTabs) {
    ze_device_module_properties_t moduleProps = {ZE_STRUCTURE_TYPE_DEVICE_MODULE_PROPERTIES};
    if (zeDeviceGetModuleProperties(device, &moduleProps) != ZE_RESULT_SUCCESS) {
        std::cerr << "zeDeviceGetModuleProperties failed\n";
        return -1;
    }
    std::string tab(numberOfTabs, '\t');
    std::string modFlags;
    if (moduleProps.flags == 0) {
        modFlags = "NONE";
    } else {
        if (moduleProps.flags & ZE_DEVICE_MODULE_FLAG_FP16)
            modFlags += "FP16 ";
        if (moduleProps.flags & ZE_DEVICE_MODULE_FLAG_FP64)
            modFlags += "FP64 ";
        if (moduleProps.flags & ZE_DEVICE_MODULE_FLAG_INT64_ATOMICS)
            modFlags += "INT64_ATOMICS ";
        if (moduleProps.flags & ZE_DEVICE_MODULE_FLAG_DP4A)
            modFlags += "DP4A ";
    }
    std::cout << tab << "Module properties:\n"
              << tab << "\tspirv version:    " << ZE_MAJOR_VERSION(moduleProps.spirvVersionSupported)
              << "." << ZE_MINOR_VERSION(moduleProps.spirvVersionSupported) << "\n"
              << tab << "\tflags:            " << modFlags << "\n"
              << tab << "\tfp16 flags:       " << parseFpFlags(moduleProps.fp16flags) << "\n"
              << tab << "\tfp32 flags:       " << parseFpFlags(moduleProps.fp32flags) << "\n"
              << tab << "\tfp64 flags:       " << parseFpFlags(moduleProps.fp64flags) << "\n"
              << tab << "\tmaxArgumentsSize: " << moduleProps.maxArgumentsSize << "\n"
              << tab << "\tprintfBufferSize: " << moduleProps.printfBufferSize << "\n";
    return 0;
}

int printImageProperties(ze_device_handle_t device, uint32_t numberOfTabs) {
    ze_device_image_properties_t imageProps = {ZE_STRUCTURE_TYPE_DEVICE_IMAGE_PROPERTIES};
    if (zeDeviceGetImageProperties(device, &imageProps) != ZE_RESULT_SUCCESS) {
        std::cerr << "zeDeviceGetImageProperties failed\n";
        return -1;
    }
    std::string tab(numberOfTabs, '\t');
    std::cout << tab << "Image properties:\n"
              << tab << "\tmaxImageDims1D:      " << imageProps.maxImageDims1D << "\n"
              << tab << "\tmaxImageDims2D:      " << imageProps.maxImageDims2D << "\n"
              << tab << "\tmaxImageDims3D:      " << imageProps.maxImageDims3D << "\n"
              << tab << "\tmaxImageBufferSize:  " << imageProps.maxImageBufferSize << "\n"
              << tab << "\tmaxImageArraySlices: " << imageProps.maxImageArraySlices << "\n"
              << tab << "\tmaxSamplers:         " << imageProps.maxSamplers << "\n"
              << tab << "\tmaxReadImageArgs:    " << imageProps.maxReadImageArgs << "\n"
              << tab << "\tmaxWriteImageArgs:   " << imageProps.maxWriteImageArgs << "\n";
    return 0;
}

int printCacheProperties(ze_device_handle_t device, uint32_t numberOfTabs) {
    uint32_t count = 0;
    if (zeDeviceGetCacheProperties(device, &count, nullptr) != ZE_RESULT_SUCCESS || count == 0) {
        return 0;
    }
    std::vector<ze_device_cache_properties_t> cacheProps(count, {ZE_STRUCTURE_TYPE_DEVICE_CACHE_PROPERTIES});
    if (zeDeviceGetCacheProperties(device, &count, cacheProps.data()) != ZE_RESULT_SUCCESS) {
        std::cerr << "zeDeviceGetCacheProperties failed\n";
        return -1;
    }
    std::string tab(numberOfTabs, '\t');
    std::cout << tab << "Cache properties (" << count << "):\n";
    for (uint32_t i = 0; i < count; i++) {
        std::string cacheFlags;
        if (cacheProps[i].flags & ZE_DEVICE_CACHE_PROPERTY_FLAG_USER_CONTROL)
            cacheFlags += "USER_CONTROL ";
        if (cacheFlags.empty())
            cacheFlags = "NONE";
        std::cout << tab << "\t[" << i << "] size: " << cacheProps[i].cacheSize
                  << "  flags: " << cacheFlags << "\n";
    }
    return 0;
}

int printDeviceProperties(ze_device_handle_t device, uint32_t numberOfTabs) {
    ze_device_properties_t deviceProperties = {};
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
    std::ios_base::fmtflags coutFlagsBackup(std::cout.flags());
    std::cout << tabDelimiter << "\tname:        " << deviceProperties.name << "\n"
              << tabDelimiter << "\tslices: " << deviceProperties.numSlices << "\n"
              << tabDelimiter << "\tsubslices per slice: " << deviceProperties.numSubslicesPerSlice << "\n"
              << tabDelimiter << "\tEUs per slice: " << deviceProperties.numEUsPerSubslice << "\n"
              << tabDelimiter << "\tthreads per EU: " << deviceProperties.numThreadsPerEU << "\n"
              << tabDelimiter << "\tdeviceId:    0x"
              << std::setw(4) << std::hex << std::setfill('0')
              << deviceProperties.deviceId << "\n"
              << tabDelimiter << "\tsubdeviceId: " << deviceProperties.subdeviceId << "\n"
              << tabDelimiter << "\tUUID: ";
    for (uint32_t i = 0; i < ZE_MAX_DEVICE_UUID_SIZE; i++) {
        std::cout << static_cast<uint32_t>(deviceProperties.uuid.id[i]) << " ";
    }
    std::cout << "\n";
    // Print each flag as a string
    std::string flagStr;
    ze_device_property_flags_t flags = deviceProperties.flags;
    if (flags == 0) {
        flagStr += " NONE";
    } else {
        if (flags & ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE)
            flagStr += " ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE";
        if (flags & ZE_DEVICE_PROPERTY_FLAG_INTEGRATED)
            flagStr += " ZE_DEVICE_PROPERTY_FLAG_INTEGRATED";
        if (flags & ZE_DEVICE_PROPERTY_FLAG_ECC)
            flagStr += " ZE_DEVICE_PROPERTY_FLAG_ECC";
        if (flags & ZE_DEVICE_PROPERTY_FLAG_ONDEMANDPAGING)
            flagStr += " ZE_DEVICE_PROPERTY_FLAG_ONDEMANDPAGING";
    }
    std::cout << tabDelimiter << "\tflags: [" << flagStr << " ]\n";

    auto memoryTypeToString = [](ze_device_memory_ext_type_t type) -> std::string {
        switch (type) {
        case ZE_DEVICE_MEMORY_EXT_TYPE_HBM:
            return "HBM";
        case ZE_DEVICE_MEMORY_EXT_TYPE_HBM2:
            return "HBM2";
        case ZE_DEVICE_MEMORY_EXT_TYPE_DDR:
            return "DDR";
        case ZE_DEVICE_MEMORY_EXT_TYPE_DDR2:
            return "DDR2";
        case ZE_DEVICE_MEMORY_EXT_TYPE_DDR3:
            return "DDR3";
        case ZE_DEVICE_MEMORY_EXT_TYPE_DDR4:
            return "DDR4";
        case ZE_DEVICE_MEMORY_EXT_TYPE_DDR5:
            return "DDR5";
        case ZE_DEVICE_MEMORY_EXT_TYPE_LPDDR:
            return "LPDDR";
        case ZE_DEVICE_MEMORY_EXT_TYPE_LPDDR3:
            return "LPDDR3";
        case ZE_DEVICE_MEMORY_EXT_TYPE_LPDDR4:
            return "LPDDR4";
        case ZE_DEVICE_MEMORY_EXT_TYPE_LPDDR5:
            return "LPDDR5";
        case ZE_DEVICE_MEMORY_EXT_TYPE_SRAM:
            return "SRAM";
        case ZE_DEVICE_MEMORY_EXT_TYPE_L1:
            return "L1";
        case ZE_DEVICE_MEMORY_EXT_TYPE_L3:
            return "L3";
        case ZE_DEVICE_MEMORY_EXT_TYPE_GRF:
            return "GRF";
        case ZE_DEVICE_MEMORY_EXT_TYPE_SLM:
            return "SLM";
        case ZE_DEVICE_MEMORY_EXT_TYPE_GDDR4:
            return "GDDR4";
        case ZE_DEVICE_MEMORY_EXT_TYPE_GDDR5:
            return "GDDR5";
        case ZE_DEVICE_MEMORY_EXT_TYPE_GDDR5X:
            return "GDDR5X";
        case ZE_DEVICE_MEMORY_EXT_TYPE_GDDR6:
            return "GDDR6";
        case ZE_DEVICE_MEMORY_EXT_TYPE_GDDR6X:
            return "GDDR6X";
        case ZE_DEVICE_MEMORY_EXT_TYPE_GDDR7:
            return "GDDR7";
        case ZE_DEVICE_MEMORY_EXT_TYPE_HBM2E:
            return "HBM2E";
        case ZE_DEVICE_MEMORY_EXT_TYPE_HBM3:
            return "HBM3";
        case ZE_DEVICE_MEMORY_EXT_TYPE_HBM3E:
            return "HBM3E";
        case ZE_DEVICE_MEMORY_EXT_TYPE_HBM4:
            return "HBM4";
        default:
            return "UNKNOWN";
        }
    };
    auto bandwidthUnitToString = [](ze_bandwidth_unit_t unit) -> std::string {
        switch (unit) {
        case ZE_BANDWIDTH_UNIT_BYTES_PER_NANOSEC:
            return "bytes/ns";
        case ZE_BANDWIDTH_UNIT_BYTES_PER_CLOCK:
            return "bytes/clock";
        default:
            return "unknown";
        }
    };

    uint32_t count = 0u;
    res = zeDeviceGetMemoryProperties(device, &count, nullptr);
    if (res == ZE_RESULT_SUCCESS && count > 0) {
        std::vector<ze_device_memory_ext_properties_t> memoryExtProperties(count);
        std::vector<ze_device_memory_properties_t> memoryProperties(count);
        for (uint32_t i = 0; i < count; i++) {
            memoryExtProperties[i].stype = ZE_STRUCTURE_TYPE_DEVICE_MEMORY_EXT_PROPERTIES;
            memoryExtProperties[i].pNext = nullptr;
            memoryProperties[i].stype = ZE_STRUCTURE_TYPE_DEVICE_MEMORY_PROPERTIES;
            memoryProperties[i].pNext = &memoryExtProperties[i];
        }
        if (zeDeviceGetMemoryProperties(device, &count, memoryProperties.data()) != ZE_RESULT_SUCCESS) {
            std::cerr << "zeDeviceGetMemoryProperties failed\n";
            return -1;
        }
        std::cout << tabDelimiter << "Memory properties (" << count << "):\n";
        for (uint32_t i = 0; i < count; i++) {
            std::cout << tabDelimiter << "\t[" << i << "] name:           " << memoryProperties[i].name << "\n";
            std::cout << tabDelimiter << "\t[" << i << "] total size:     " << std::dec << memoryProperties[i].totalSize << "\n";
            std::cout << tabDelimiter << "\t[" << i << "] max clock rate: " << memoryProperties[i].maxClockRate << "\n";
            std::cout << tabDelimiter << "\t[" << i << "] type:           " << memoryTypeToString(memoryExtProperties[i].type) << "\n";
            std::cout << tabDelimiter << "\t[" << i << "] physical size:  " << memoryExtProperties[i].physicalSize << "\n";
            std::cout << tabDelimiter << "\t[" << i << "] read BW:        " << memoryExtProperties[i].readBandwidth
                      << " " << bandwidthUnitToString(memoryExtProperties[i].bandwidthUnit) << "\n";
            std::cout << tabDelimiter << "\t[" << i << "] write BW:       " << memoryExtProperties[i].writeBandwidth
                      << " " << bandwidthUnitToString(memoryExtProperties[i].bandwidthUnit) << "\n";
        }
    }

    ze_device_memory_access_properties_t memoryAccessCapabilities = {};

    res = zeDeviceGetMemoryAccessProperties(device, &memoryAccessCapabilities);

    if (res != ZE_RESULT_SUCCESS) {
        std::cerr << "zeDeviceGetMemoryAccessProperties failed\n";
        return -1;
    }
    std::cout << tabDelimiter << "Memory access capabilities: \n";
    std::cout << tabDelimiter << "\thost: " << parseCaps(memoryAccessCapabilities.hostAllocCapabilities) << "\n";
    std::cout << tabDelimiter << "\tdevice: " << parseCaps(memoryAccessCapabilities.deviceAllocCapabilities) << "\n";
    std::cout << tabDelimiter << "\tshared single device: " << parseCaps(memoryAccessCapabilities.sharedSingleDeviceAllocCapabilities) << "\n";
    std::cout << tabDelimiter << "\tshared cross device: " << parseCaps(memoryAccessCapabilities.sharedCrossDeviceAllocCapabilities) << "\n";
    std::cout << tabDelimiter << "\tshared system caps: " << parseCaps(memoryAccessCapabilities.sharedSystemAllocCapabilities) << "\n";

    std::cout.flags(coutFlagsBackup);
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

    ret = printComputeProperties(device, numberOfTabs);
    if (ret) {
        return -1;
    }

    ret = printModuleProperties(device, numberOfTabs);
    if (ret) {
        return -1;
    }

    ret = printImageProperties(device, numberOfTabs);
    if (ret) {
        return -1;
    }

    ret = printCacheProperties(device, numberOfTabs);
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
    res = zeDeviceGetSubDevices(device, &subDeviceCount, subDevices.data());
    if (res) {
        std::cerr << "zeDeviceGetSubDevices failed\n";
        return -1;
    }
    for (auto subDevice : subDevices) {
        ret = printPropertiesForAllSubDevices(subDevice, numberOfTabs + 1);
        if (ret) {
            return -1;
        }
    }
    return 0;
}

struct deviceAndProperty {
    ze_pci_ext_properties_t properties = {};
    ze_device_handle_t deviceHandle;
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
        properties.at(dataIdentifier).deviceHandle = device;
        properties.at(dataIdentifier).deviceIndex = dataIdentifier;
        ze_result_t res = zeDevicePciGetPropertiesExt(device, &properties.at(dataIdentifier++).properties);

        if (res) {
            std::cerr << "zeDevicePciGetPropertiesExt failed\n";
            return -1;
        }
    }

    // locate devices under the same PCI BUS
    std::map<std::string, std::vector<uint32_t>> pciIdentifiers;

    for (const auto &currentPciProp : properties) {
        auto domainId = currentPciProp.properties.address.domain;
        auto busId = currentPciProp.properties.address.bus;
        auto deviceId = currentPciProp.properties.address.device;
        auto functionId = currentPciProp.properties.address.function;
        std::ostringstream pciBdf{};
        pciBdf << std::hex << std::setfill('0') << std::setw(4) << domainId << ":" << std::setw(2) << busId << ":" << std::setw(2) << deviceId << ":" << functionId;

        if (pciIdentifiers.find(pciBdf.str()) != pciIdentifiers.end()) {
            continue;
        }
        std::vector<uint32_t> deviceIndexes{};

        for (const auto &property : properties) {
            if (domainId == property.properties.address.domain &&
                busId == property.properties.address.bus &&
                deviceId == property.properties.address.device &&
                functionId == property.properties.address.function) {
                deviceIndexes.push_back(property.deviceIndex);
            }
        }
        pciIdentifiers.insert(std::make_pair(pciBdf.str(), deviceIndexes));
    }

    for (auto &devicePciIdentifier : pciIdentifiers) {
        auto pciProp = devicePciIdentifier.first;
        std::cout << "PCI: " << devicePciIdentifier.first
                  << "\n";
        std::cout << "\t|\n";
        std::cout << "\t|\n";
        for (const auto &handle : devicePciIdentifier.second) {
            std::cout << "\t___________ Device " << std::dec << handle << "\n";
        }
    }

    return 0u;
}

int main() {
    Configuration::loadDefaultConfiguration();

    ze_init_driver_type_desc_t initDesc = {};
    initDesc.stype = ZE_STRUCTURE_TYPE_INIT_DRIVER_TYPE_DESC;
    initDesc.pNext = nullptr;
    initDesc.flags = UINT32_MAX;
    uint32_t driverCount = 0;
    ze_result_t res = zeInitDrivers(&driverCount, nullptr, &initDesc);
    if (driverCount == 0 || res != ZE_RESULT_SUCCESS) {
        std::cerr << "zeInitDrivers failed\n";
        return -1;
    }
    std::vector<ze_driver_handle_t> drivers(driverCount);
    res = zeInitDrivers(&driverCount, drivers.data(), &initDesc);
    if (res != ZE_RESULT_SUCCESS) {
        std::cerr << "zeInitDrivers failed during driver handle retrieval\n";
        return -1;
    }

    for (auto driver : drivers) {
        uint32_t deviceCount = 0;
        res = zeDeviceGet(driver, &deviceCount, nullptr);
        if (deviceCount == 0 || res != ZE_RESULT_SUCCESS) {
            std::cout << "No devices found\n";
            continue;
        }
        ze_driver_properties_t driverProperties = {ZE_STRUCTURE_TYPE_DRIVER_PROPERTIES};

        EXPECT_ZE_RESULT_SUCCESS(zeDriverGetProperties(driver, &driverProperties));
        std::cout << "Driver Version: " << (driverProperties.driverVersion & 0xffff) << "\n ";

        std::vector<ze_device_handle_t> devices(deviceCount);
        EXPECT_ZE_RESULT_SUCCESS(zeDeviceGet(driver, &deviceCount, devices.data()));

        for (auto device : devices) {
            int ret = printPropertiesForAllSubDevices(device, 0);
            if (ret) {
                return -1;
            }
        }
        int ret = printDevicesWithTheSameBdfAddress(devices);
        if (ret) {
            return -1;
        }
    }

    return 0;
}
