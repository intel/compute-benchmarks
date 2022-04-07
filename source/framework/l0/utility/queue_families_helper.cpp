/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "queue_families_helper.h"

namespace L0 {

std::unique_ptr<QueueFamiliesHelper::QueueDesc> QueueFamiliesHelper::getPropertiesForSelectingEngine(ze_device_handle_t device, Engine engine) {
    const EngineGroup engineGroup = EngineHelper::getEngineGroup(engine);
    const size_t engineIndex = EngineHelper::getEngineIndexWithinGroup(engine);

    const auto &families = queryQueueFamilies(device);
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
    for (uint32_t familyIndex = 0; familyIndex < familiesCount; familyIndex++) {
        const ze_command_queue_group_properties_t properties = families[familyIndex];
        const EngineGroup engineGroup = getEngineGroup(properties);

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

EngineGroup QueueFamiliesHelper::getEngineGroup(ze_command_queue_group_properties_t family) {
    const bool isCompute = family.flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE;
    const bool isCopy = family.flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY;
    const bool isNoneQueue = family.numQueues == 0;
    const bool isSingleQueue = family.numQueues == 1;

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
        if (isSingleQueue) {
            return EngineGroup::Copy; //  this could also be Linked BCS with only 1 queue, no way to know this
        } else {
            return EngineGroup::LinkCopy;
        }
    }

    return EngineGroup::Unknown;
}
} // namespace L0
