/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/enum/device_selection.h"
#include "framework/enum/usm_initial_placement.h"
#include "framework/enum/usm_memory_placement.h"
#include "framework/ocl/opencl.h"

struct UsmHelperOcl {
    static constexpr auto misalignedOffset = 4u;
    struct Alloc {
        void *ptr = nullptr;
        UsmMemoryPlacement placement = UsmMemoryPlacement::Unknown;
        UsmFunctions usm = {};
        cl_context context = {};

        struct MappedData {
            cl_mem memObject = {};
            cl_command_queue queue = {};
        } mappedData;
    };

    static cl_int allocate(Opencl &opencl,
                           UsmMemoryPlacement placement,
                           size_t bufferSize,
                           Alloc &outAlloc);
    static cl_int deallocate(Alloc &alloc);

    static void *allocate(DeviceSelection placement,
                          Opencl &opencl,
                          size_t bufferSize,
                          cl_int *retVal);

    static cl_mem_properties_intel getInitialPlacementFlag(UsmInitialPlacement placement);
};
