/*
 * Copyright (C) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include <chrono>
#include <framework/configuration.h>
#include <level_zero/zes_api.h>
class PowerMeter {
  public:
    PowerMeter() {
        if (Configuration::get().measurePower) {
            this->powerHandle = this->getPowerHandle();
            this->enabled = !!this->powerHandle;
            if (!this->isEnabled()) {
                fprintf(stderr, "Requested to measure power but could not obtain power handle from sysman API. Power will not be measured.");
            }
        }
    }

    bool isEnabled() {
        return this->enabled;
    }

    ze_result_t measureStart() {
        if (this->isEnabled()) {
            return zesPowerGetEnergyCounter(powerHandle, &this->energyStart);
        }
        return ZE_RESULT_SUCCESS;
    }

    ze_result_t measureEnd() {
        if (this->isEnabled()) {
            return zesPowerGetEnergyCounter(powerHandle, &this->energyEnd);
        }
        return ZE_RESULT_SUCCESS;
    }

    uint64_t getEnergy() {
        return energyEnd.energy - energyStart.energy;
    }

    std::chrono::microseconds getTime() {
        return std::chrono::microseconds(getRawTime());
    }

    double getPower() {
        return (this->getEnergy() * 1.0) / this->getRawTime();
    }

  protected:
    uint64_t getRawTime() {
        return this->energyEnd.timestamp - this->energyStart.timestamp;
    }

    zes_pwr_handle_t getPowerHandle() {
        zes_init_flags_t zesFlags{};
        if (ZE_RESULT_SUCCESS != zesInit(zesFlags)) {
            return nullptr;
        }

        uint32_t pCount = 0;
        zes_driver_handle_t sysmanDriverHandle{};
        if (ZE_RESULT_SUCCESS != zesDriverGet(&pCount, nullptr) || !pCount) {
            return nullptr;
        }

        pCount = 1;
        if (ZE_RESULT_SUCCESS != zesDriverGet(&pCount, &sysmanDriverHandle) || !sysmanDriverHandle) {
            return nullptr;
        }

        zes_device_handle_t sysmanDeviceHandle{};
        if (ZE_RESULT_SUCCESS != zesDeviceGet(sysmanDriverHandle, &pCount, &sysmanDeviceHandle) || !sysmanDeviceHandle) {
            return nullptr;
        }

        if (ZE_RESULT_SUCCESS != zesDeviceEnumPowerDomains(sysmanDeviceHandle, &pCount, nullptr) || !pCount) {
            return nullptr;
        }

        pCount = 1;
        zes_pwr_handle_t powerHandleReturned{};
        if (ZE_RESULT_SUCCESS != zesDeviceEnumPowerDomains(sysmanDeviceHandle, &pCount, &powerHandleReturned)) {
            return nullptr;
        }
        return powerHandleReturned;
    }

    bool enabled{false};
    zes_pwr_handle_t powerHandle{};
    zes_power_energy_counter_t energyStart{}, energyEnd{};
};