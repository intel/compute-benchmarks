/*
 * Copyright (C) 2022-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/enum/engine.h"
#include "framework/ocl/cl.h"
#include "framework/ocl/utility/error.h"

#include <memory>
#include <vector>

class QueueFamiliesHelper {
  public:
    struct PropertiesForSelectingQueue {
        cl_queue_properties properties[4];
        cl_uint propertiesCount;
    };

    static inline std::unique_ptr<PropertiesForSelectingQueue> getPropertiesForSelectingEngine(cl_device_id device, Engine engine) {
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

            auto result = std::make_unique<PropertiesForSelectingQueue>();
            result->properties[0] = CL_QUEUE_FAMILY_INTEL;
            result->properties[1] = queueFamilyDesc.familyIndex;
            result->properties[2] = CL_QUEUE_INDEX_INTEL;
            result->properties[3] = engineIndex;
            result->propertiesCount = 4;
            return result;
        }
        return nullptr;
    }

    static size_t getQueueCountForEngineGroup(cl_device_id clDevice, EngineGroup engineGroup) {
        size_t queueCount{0};
        const auto &families = queryQueueFamilies(clDevice);
        for (const auto &queueFamilyDesc : families) {
            if (queueFamilyDesc.type == engineGroup) {
                queueCount += queueFamilyDesc.queueCount;
            }
        }
        return queueCount;
    }

    static bool validateCapability(cl_command_queue queue, cl_command_queue_capabilities_intel capability) {
        return validateCapability(getQueueCapabilities(queue), capability);
    }

    template <typename... Args>
    static bool validateCapabilities(cl_command_queue queue, Args &&...args) {
        return validateCapabilities(getQueueCapabilities(queue), std::forward<Args>(args)...);
    }

  private:
    struct QueueFamilyDesc {
        size_t familyIndex;
        cl_command_queue_capabilities_intel capabilitites;
        size_t queueCount;
        EngineGroup type;
    };

    static inline std::vector<QueueFamilyDesc> queryQueueFamilies(cl_device_id device) {
        // Get families count
        size_t familyPropertiesSize{};
        cl_int retVal = clGetDeviceInfo(device, CL_DEVICE_QUEUE_FAMILY_PROPERTIES_INTEL, 0, nullptr, &familyPropertiesSize);
        if (retVal != CL_SUCCESS) {
            return {};
        }
        size_t familiesCount = familyPropertiesSize / sizeof(cl_queue_family_properties_intel);
        if (familiesCount == 0 || familiesCount * sizeof(cl_queue_family_properties_intel) != familyPropertiesSize) {
            return {};
        }

        // Get families
        auto families = std::make_unique<cl_queue_family_properties_intel[]>(familiesCount);
        retVal = clGetDeviceInfo(device, CL_DEVICE_QUEUE_FAMILY_PROPERTIES_INTEL, familyPropertiesSize, families.get(), nullptr);
        if (retVal != CL_SUCCESS) {
            return {};
        }

        std::vector<QueueFamilyDesc> result = {};
        for (auto familyIndex = 0u; familyIndex < familiesCount; familyIndex++) {
            QueueFamilyDesc desc{};
            desc.familyIndex = familyIndex;
            desc.capabilitites = families[familyIndex].capabilities;
            desc.queueCount = families[familyIndex].count;
            desc.type = EngineHelper::parseEngineGroup(families[familyIndex].name);
            result.push_back(desc);
        }

        return result;
    }

    static inline size_t getCopyEnginesCount(const std::vector<QueueFamilyDesc> &families) {
        for (const auto &family : families) {
            if (family.type == EngineGroup::Copy) {
                return family.queueCount;
            }
        }
        return 0;
    }

    static cl_command_queue_capabilities_intel getQueueCapabilities(cl_command_queue queue) {
        cl_uint familyIndex = {};
        cl_int retVal = clGetCommandQueueInfo(queue, CL_QUEUE_FAMILY_INTEL, sizeof(familyIndex), &familyIndex, nullptr);
        if (retVal != CL_SUCCESS) {
            return CL_QUEUE_DEFAULT_CAPABILITIES_INTEL;
        }
        cl_device_id device = {};
        EXPECT_CL_SUCCESS(clGetCommandQueueInfo(queue, CL_QUEUE_DEVICE, sizeof(device), &device, nullptr));

        const auto families = queryQueueFamilies(device);
        const auto &family = families[familyIndex];
        return family.capabilitites;
    }

    static bool validateCapability(cl_command_queue_capabilities_intel queueCapabilities, cl_command_queue_capabilities_intel capability) {
        return queueCapabilities == CL_QUEUE_DEFAULT_CAPABILITIES_INTEL || ((queueCapabilities & capability) == capability);
    }

    template <typename... Args>
    static bool validateCapabilities(cl_command_queue_capabilities_intel queueCapabilities, cl_command_queue_capabilities_intel capability, Args &&...args) {
        if (!validateCapability(queueCapabilities, capability)) {
            return false;
        }
        if constexpr (sizeof...(Args) > 0) {
            return validateCapabilities(queueCapabilities, std::forward<Args>(args)...);
        } else {
            return true;
        }
    }
};
