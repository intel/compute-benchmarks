/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/process_group.h"

#include "definitions/multi_process_init.h"

#include <gtest/gtest.h>

static TestResult run(const MultiProcessInitArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    ProcessGroup processes{"init_workload_l0", arguments.numberOfProcesses};
    processes.addArgumentAll("initFlag", std::to_string(arguments.initFlag));

    for (auto i = 0u; i < processes.size(); i++) {
        processes[i].setName("L0 Init Process #" + std::to_string(i));
    }

    processes.runAll();
    processes.synchronizeAll(arguments.iterations);
    processes.waitForFinishAll();

    TestResult result = processes.getResultAll();
    if (result != TestResult::Success) {
        return result;
    }

    const bool pushIndividualProcessesMeasurements = (processes.size() > 1);
    processes.pushMeasurementsToStatistics(arguments.iterations, statistics, typeSelector.getUnit(),
                                           typeSelector.getType(), pushIndividualProcessesMeasurements, true);

    return TestResult::Success;
}

static RegisterTestCaseImplementation<MultiProcessInit> registerTestCase(run, Api::L0);
