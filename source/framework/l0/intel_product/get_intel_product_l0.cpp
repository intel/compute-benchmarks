/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "get_intel_product_l0.h"

#include "framework/intel_product/get_intel_product.h"
#include "framework/l0/levelzero.h"

IntelProduct getIntelProduct(ze_device_handle_t device) {
    ze_device_properties_t properties{};
    ze_result_t retVal = zeDeviceGetProperties(device, &properties);
    if (retVal != ZE_RESULT_SUCCESS) {
        return IntelProduct::Unknown;
    }
    return getIntelProduct(static_cast<uint32_t>(properties.deviceId));
}

IntelProduct getIntelProduct(const ze_device_properties_t &deviceProperties) {
    return getIntelProduct(deviceProperties.deviceId);
}

IntelProduct getIntelProduct(const LevelZero &levelzero) {
    return getIntelProduct(levelzero.getDeviceProperties());
}
