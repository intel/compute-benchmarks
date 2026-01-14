/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/enum/engine.h"
#include "framework/enum/priority_level.h"

#include <level_zero/ze_api.h>

namespace L0 {
struct QueueProperties {
    bool createQueue = true;
    bool requireCreationSuccess = true;
    Engine selectedEngine = Engine::Ccs0;
    DeviceSelection deviceSelection = DeviceSelection::Unknown;
    ze_command_queue_priority_t priority = ZE_COMMAND_QUEUE_PRIORITY_NORMAL;

    static QueueProperties create() {
        return QueueProperties()
            .setDeviceSelection(Configuration::get().subDeviceSelection);
    }

    QueueProperties &allowCreationFail() {
        requireCreationSuccess = false;
        return *this;
    }

    QueueProperties &setForceBlitter(bool bcs) {
        this->selectedEngine = bcs ? Engine::Bcs : Engine::Ccs0;
        return *this;
    }

    QueueProperties &setForceEngine(Engine engine) {
        this->selectedEngine = engine;
        return *this;
    }

    QueueProperties &disable() {
        this->createQueue = false;
        return *this;
    }

    QueueProperties &setDeviceSelection(DeviceSelection newValue) {
        FATAL_ERROR_IF(DeviceSelectionHelper::hasHost(newValue), "Cannot create queue on host device");
        FATAL_ERROR_UNLESS(DeviceSelectionHelper::hasSingleDevice(newValue), "Queue can be created only on a single device");
        this->deviceSelection = newValue;
        return *this;
    }

    QueueProperties &setPriority(PriorityLevel priorityLevel) {
        switch (priorityLevel) {
        case PriorityLevel::Low:
            this->priority = ZE_COMMAND_QUEUE_PRIORITY_PRIORITY_LOW;
            break;
        case PriorityLevel::Normal:
            this->priority = ZE_COMMAND_QUEUE_PRIORITY_NORMAL;
            break;
        case PriorityLevel::High:
            this->priority = ZE_COMMAND_QUEUE_PRIORITY_PRIORITY_HIGH;
            break;
        default:
            break;
        }
        return *this;
    }
};

} // namespace L0
