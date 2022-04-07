/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/enum/map_flags.h"
#include "framework/ocl/opencl.h"

namespace OCL {
inline cl_map_flags convertMapFlags(MapFlags mapFlags) {
    switch (mapFlags) {
    case MapFlags::Read:
        return CL_MAP_READ;
    case MapFlags::Write:
        return CL_MAP_WRITE;
    case MapFlags::WriteInvalidate:
        return CL_MAP_WRITE_INVALIDATE_REGION;
    default:
        FATAL_ERROR("Unknown map flag");
    }
}
} // namespace OCL
