/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/intel_product/get_intel_product_ocl.h"

#include "framework/ocl/utility/extensions_helper.h"

IntelProduct getIntelProduct(cl_device_id device) {
    ExtensionsHelper extensions{device};

    if (!extensions.isSupported("cl_intel_device_attribute_query")) {
        return IntelProduct::Unknown;
    }
    cl_uint deviceId{};
    cl_int retVal = clGetDeviceInfo(device, CL_DEVICE_ID_INTEL, sizeof(deviceId), &deviceId, nullptr);
    if (retVal != CL_SUCCESS) {
        return IntelProduct::Unknown;
    }

    return getIntelProduct(static_cast<uint32_t>(deviceId));
}

IntelProduct getIntelProduct(const Opencl &opencl) {
    return getIntelProduct(opencl.device);
}
