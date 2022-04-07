/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/configuration.h"
#include "framework/ocl/opencl.h"

namespace ProfilingHelper {

inline cl_int getEventDurationInNanoseconds(cl_event &profilingEvent, cl_ulong &timeNs) {
    cl_ulong start{};
    auto firstTimestamp = CL_PROFILING_COMMAND_START;
    auto secondTimestamp = CL_PROFILING_COMMAND_END;

    if (Configuration::get().returnSubmissionTimeInsteadOfWorkloadTime) {
        firstTimestamp = CL_PROFILING_COMMAND_QUEUED;
        secondTimestamp = CL_PROFILING_COMMAND_START;
    }

    cl_int retVal = clGetEventProfilingInfo(profilingEvent, firstTimestamp, sizeof(cl_ulong), &start, nullptr);
    if (retVal != CL_SUCCESS) {
        return retVal;
    }

    cl_ulong end{};
    retVal = clGetEventProfilingInfo(profilingEvent, secondTimestamp, sizeof(cl_ulong), &end, nullptr);
    if (retVal != CL_SUCCESS) {
        return retVal;
    }

    timeNs = end - start;
    return CL_SUCCESS;
}

} // namespace ProfilingHelper
