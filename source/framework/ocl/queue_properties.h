/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/configuration.h"
#include "framework/enum/device_selection.h"
#include "framework/enum/engine.h"
#include "framework/ocl/cl.h"
#include "framework/ocl/utility/queue_families_helper.h"

namespace OCL {
struct QueueProperties {
    bool createQueue = true;
    bool requireCreationSuccess = true;
    bool profiling = false;
    Engine selectedEngine = Engine::Unknown;
    int ooq = -1;
    DeviceSelection deviceSelection = DeviceSelection::Unknown;

    static QueueProperties create() {
        return QueueProperties()
            .setDeviceSelection(Configuration::get().subDeviceSelection);
    }

    QueueProperties &setProfiling(bool newValue) {
        this->profiling = newValue;
        return *this;
    }

    QueueProperties &setForceBlitter(bool bcs) {
        this->selectedEngine = bcs ? Engine::Bcs : Engine::Unknown;
        return *this;
    }

    QueueProperties &setForceEngine(Engine engine) {
        this->selectedEngine = engine;
        return *this;
    }

    QueueProperties &setOoq(bool newValue) {
        this->ooq = newValue;
        return *this;
    }

    QueueProperties &disable() {
        createQueue = false;
        return *this;
    }

    QueueProperties &allowCreationFail() {
        requireCreationSuccess = false;
        return *this;
    }

    QueueProperties &setDeviceSelection(DeviceSelection newValue) {
        FATAL_ERROR_UNLESS(DeviceSelectionHelper::hasSingleDevice(newValue), "Queue can be created only on a single device");
        this->deviceSelection = newValue;
        return *this;
    }

    bool fillQueueProperties(cl_device_id device, cl_queue_properties properties[], size_t size) const {
        std::fill_n(properties, size, 0);
        properties[0] = CL_QUEUE_PROPERTIES;
        cl_int propertiesIndex = 2;
        if (this->profiling) {
            properties[1] |= CL_QUEUE_PROFILING_ENABLE;
        }
        if (this->ooq == 1 || (this->ooq == -1 && Configuration::get().useOOQ)) {
            properties[1] |= CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE;
        }
        if (this->selectedEngine != Engine::Unknown) {
            const auto propertiesForBlitter = QueueFamiliesHelper::getPropertiesForSelectingEngine(device, this->selectedEngine);
            if (propertiesForBlitter == nullptr) {
                return false;
            }

            for (auto i = 0u; i < propertiesForBlitter->propertiesCount; i++) {
                properties[propertiesIndex++] = propertiesForBlitter->properties[i];
            }
        }

        return true;
    }
};
} // namespace OCL
