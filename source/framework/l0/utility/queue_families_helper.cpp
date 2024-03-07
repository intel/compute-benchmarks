/*
 * Copyright (C) 2022-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "queue_families_helper.h"

namespace L0 {

std::unique_ptr<QueueFamiliesHelper::QueueDesc> QueueFamiliesHelper::getPropertiesForSelectingEngine(ze_device_handle_t device, Engine engine) {
    const auto &families = queryQueueFamilies(device);
    const auto copyEnginesCount = getCopyEnginesCount(families);

    const EngineGroup engineGroup = EngineHelper::getEngineGroup(engine, copyEnginesCount);
    const size_t engineIndex = EngineHelper::getEngineIndexWithinGroup(engine, copyEnginesCount);

    for (const auto &queueFamilyDesc : families) {
        if (engineGroup != queueFamilyDesc.type) {
            continue;
        }
        if (engineIndex >= queueFamilyDesc.queueCount) {
            continue;
        }

        auto result = std::make_unique<QueueDesc>();
        result->family = queueFamilyDesc;
        result->desc.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC;
        result->desc.pNext = nullptr;
        result->desc.ordinal = queueFamilyDesc.ordinal;
        result->desc.index = static_cast<uint32_t>(engineIndex);
        result->desc.flags = 0;
        result->desc.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
        result->desc.priority = ZE_COMMAND_QUEUE_PRIORITY_NORMAL;
        return result;
    }
    return nullptr;
}

std::vector<QueueFamiliesHelper::QueueFamilyDesc> QueueFamiliesHelper::queryQueueFamilies(ze_device_handle_t device) {
    // Get queue ordinals
    uint32_t familiesCount = 0;
    EXPECT_ZE_RESULT_SUCCESS(zeDeviceGetCommandQueueGroupProperties(device, &familiesCount, nullptr));
    FATAL_ERROR_IF(familiesCount == 0, "No queue groups found!");
    std::vector<ze_command_queue_group_properties_t> families(familiesCount);
    EXPECT_ZE_RESULT_SUCCESS(zeDeviceGetCommandQueueGroupProperties(device, &familiesCount, families.data()));

    // Iterate over queue groups
    std::vector<QueueFamilyDesc> result{};
    bool copyEngineFound = false;
    for (uint32_t familyIndex = 0; familyIndex < familiesCount; familyIndex++) {
        const ze_command_queue_group_properties_t properties = families[familyIndex];
        const EngineGroup engineGroup = getEngineGroup(properties, copyEngineFound);

        if (engineGroup == EngineGroup::Unknown) {
            DEVELOPER_WARNING("Unknown LevelZero queue group at index. Ignoring.");
            continue;
        }

        QueueFamilyDesc family{};
        family.device = device;
        family.ordinal = familyIndex;
        family.maxFillSize = properties.maxMemoryFillPatternSize;
        family.queueCount = properties.numQueues;
        family.type = engineGroup;
        result.push_back(family);
    }
    return result;
}

EngineGroup QueueFamiliesHelper::getEngineGroup(const ze_command_queue_group_properties_t family, bool &copyEngineFound) {
    const bool isCompute = family.flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE;
    const bool isCopy = family.flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY;
    const bool isNoneQueue = family.numQueues == 0;

    if (isNoneQueue) {
        return EngineGroup::Unknown;
    }

    if (isCompute) {
        if (isCopy) {
            return EngineGroup::Compute; // this could also be RCS, no way to know this
        } else {
            return EngineGroup::Unknown;
        }
    }

    if (isCopy) {
        if (copyEngineFound) {
            return EngineGroup::LinkCopy;
        } else {
            copyEngineFound = true;
            return EngineGroup::Copy; // Assuming main copy engines are reported before link copy
        }
    }

    return EngineGroup::Unknown;
}

size_t QueueFamiliesHelper::getCopyEnginesCount(const std::vector<QueueFamiliesHelper::QueueFamilyDesc> &families) {
    for (const auto &family : families) {
        if (family.type == EngineGroup::Copy) {
            return family.queueCount;
        }
    }
    return 0;
}

} // namespace L0
