/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/ocl/cl.h"

#include <CL/cl_ext_intel.h>
#include <algorithm>
#include <initializer_list>
#include <vector>

namespace OCL {
struct SubGroupHelper {
    // Sub-group sizes the device can be asked for via intel_reqd_sub_group_size.
    // Returns an empty vector when the device does not expose the query.
    static std::vector<size_t> getSupportedSubGroupSizes(cl_device_id device) {
        size_t infoSize{};
        if (CL_SUCCESS != clGetDeviceInfo(device, CL_DEVICE_SUB_GROUP_SIZES_INTEL, 0, nullptr, &infoSize) || infoSize == 0) {
            return {};
        }

        std::vector<size_t> subGroupSizes(infoSize / sizeof(size_t));
        if (CL_SUCCESS != clGetDeviceInfo(device, CL_DEVICE_SUB_GROUP_SIZES_INTEL, infoSize, subGroupSizes.data(), nullptr)) {
            return {};
        }

        return subGroupSizes;
    }

    // Returns the first of preferredSizes supported by the device, in the order given.
    // Returns the first entry when the device does not expose the query, and 0 when the
    // device exposes it but supports none of preferredSizes.
    static size_t selectSubGroupSize(cl_device_id device, std::initializer_list<size_t> preferredSizes) {
        if (preferredSizes.size() == 0) {
            return 0;
        }

        const auto supportedSizes = getSupportedSubGroupSizes(device);
        if (supportedSizes.empty()) {
            return *preferredSizes.begin();
        }

        for (const auto preferredSize : preferredSizes) {
            if (std::find(supportedSizes.begin(), supportedSizes.end(), preferredSize) != supportedSizes.end()) {
                return preferredSize;
            }
        }

        return 0;
    }
};
} // namespace OCL
