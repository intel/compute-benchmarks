/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/enum/hostptr_reuse_mode.h"
#include "framework/ocl/function_signatures_ocl.h"
#include "framework/ocl/opencl.h"
#include "framework/test_case/test_result.h"

struct HostptrReuseHelper {
    static constexpr size_t misalignedOffset = 4u;

    struct Alloc {
        void *ptr = nullptr;

        HostptrReuseMode reuseMode = HostptrReuseMode::Unknown;
        cl_mem memObject = nullptr;
        cl_command_queue queue = nullptr;
        cl_context context = nullptr;
        pfn_clMemFreeINTEL clMemFreeINTEL = nullptr;
    };

    static cl_int allocateBufferHostptr(Opencl &opencl,
                                        HostptrReuseMode reuseMode,
                                        size_t size,
                                        Alloc &outAlloc);
    static cl_int deallocateBufferHostptr(Alloc alloc);
};
