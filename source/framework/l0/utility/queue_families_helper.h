/*
 * Copyright (C) 2022-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/enum/engine.h"
#include "framework/l0/utility/error.h"

#include <level_zero/ze_api.h>
#include <memory>
#include <vector>

namespace L0 {
class QueueFamiliesHelper {
  public:
    struct QueueFamilyDesc {
        ze_device_handle_t device;
        uint32_t ordinal;
        size_t maxFillSize;
        size_t queueCount;
        EngineGroup type;
    };

    struct QueueDesc {
        QueueFamilyDesc family;
        ze_command_queue_desc_t desc;
        ze_command_queue_handle_t queue;
    };

    static std::unique_ptr<QueueDesc> getPropertiesForSelectingEngine(ze_device_handle_t device, Engine engine);
    static std::vector<QueueFamilyDesc> queryQueueFamilies(ze_device_handle_t device);

  private:
    static EngineGroup getEngineGroup(const ze_command_queue_group_properties_t family, bool &copyEngineFound);
    static size_t getCopyEnginesCount(const std::vector<QueueFamilyDesc> &families);
};
} // namespace L0
