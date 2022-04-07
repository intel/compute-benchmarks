/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/ocl/cl.h"
#include "framework/test_case/test_result.h"

namespace OCL {
struct ProgramHelperOcl {
    static TestResult buildProgramFromSource(cl_context context,
                                             cl_device_id device,
                                             const char *source,
                                             size_t sourceLength,
                                             const char *compileOptions,
                                             cl_program &outProgram);

    static TestResult buildProgramFromSourceFile(cl_context context,
                                                 cl_device_id device,
                                                 const char *sourceFileName,
                                                 const char *compileOptions,
                                                 cl_program &outProgram);
};
} // namespace OCL
