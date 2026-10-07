/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "multi_process_helper.h"

#include <sstream>

std::vector<DeviceSelection> MultiProcessHelper::getSubDevicesForExecution(DeviceSelection subDevices, size_t processesPerDevice, size_t actualSubDevicesCount) {
    std::vector<DeviceSelection> subDevicesForExecution = DeviceSelectionHelper::split(subDevices);

    for (DeviceSelection subDevice : subDevicesForExecution) {
        if (DeviceSelectionHelper::getSubDeviceIndex(subDevice) >= actualSubDevicesCount) {
            return {};
        }
    }

    const auto tilesForExecutionCount = subDevicesForExecution.size();
    for (auto i = 1u; i < processesPerDevice; i++) {
        for (auto j = 0u; j < tilesForExecutionCount; j++) {
            subDevicesForExecution.push_back(subDevicesForExecution[j]);
        }
    }

    return subDevicesForExecution;
}

std::string MultiProcessHelper::createAffinityMask(size_t rootDeviceIndex, DeviceSelection subDevices) {
    std::ostringstream result{};
    const auto subDevicesSplit = DeviceSelectionHelper::split(subDevices);
    for (auto i = 0u; i < subDevicesSplit.size(); i++) {
        const auto subDeviceIndex = DeviceSelectionHelper::getSubDeviceIndex(subDevicesSplit[i]);
        result << rootDeviceIndex << '.' << subDeviceIndex;
        if (i != subDevicesSplit.size() - 1) {
            result << ",";
        }
    }
    return result.str();
}

std::string MultiProcessHelper::createProcessName(const std::vector<DeviceSelection> &subDevicesForExecution, size_t processIndex) {
    std::ostringstream processName{};
    processName << "process" << processIndex << " (" << DeviceSelectionHelper::toString(subDevicesForExecution[processIndex]) << ")";
    return processName.str();
}
