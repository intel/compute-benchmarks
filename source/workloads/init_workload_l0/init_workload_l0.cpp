/*
 * Copyright (C) 2023-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"
#include "framework/workload/register_workload.h"

struct InitArguments : WorkloadArgumentContainer {
    IntegerArgument initFlag;

    InitArguments()
        : initFlag(*this, "initFlag", "Initialization flag. For Level Zero: 0 - default, 1 - ZE_INIT_FLAG_GPU_ONLY, 2 - ZE_INIT_FLAG_VPU_ONLY") {
        synchronize = true;
    }
};

struct Init : Workload<InitArguments> {};

TestResult run(const InitArguments &arguments, Statistics &statistics, WorkloadSynchronization &synchronization, WorkloadIo &io) {
    ze_init_flags_t flags;

    switch (arguments.initFlag) {
    case 1: {
        flags = ZE_INIT_FLAG_GPU_ONLY;
        break;
    }
    case 2: {
        flags = ZE_INIT_FLAG_VPU_ONLY;
        break;
    }
    default: {
        flags = 0;
        break;
    }
    }

    Timer timer{};
    for (auto i = 0u; i < arguments.iterations; i++) {
        synchronization.synchronize(io);

        // Only the first zeInit()'s timing is important
        if (i == 0) {
            timer.measureStart();
            zeInit(flags);
            timer.measureEnd();
        }

        statistics.pushValue(timer.get(), MeasurementUnit::Unknown, MeasurementType::Unknown);
    }

    return TestResult::Success;
}

int main(int argc, char **argv) {
    Init workload;
    Init::implementation = run;
    return workload.runFromCommandLine(argc, argv);
}
