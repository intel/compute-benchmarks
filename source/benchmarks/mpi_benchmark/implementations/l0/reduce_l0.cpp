/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/enum/mpi_test.h"
#include "framework/test_case/register_test_case.h"

#include "definitions/reduce.h"
#include "utility/mpi_helper.h"

#include <gtest/gtest.h>
#include <sstream>
#include <unistd.h>
#include <vector>

static TestResult run(const MpiReduceArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    std::stringstream workloadArgs;
    workloadArgs << "--testType=" << static_cast<int>(MpiTestType::Reduce) << ' '
                 << "--testArg0=" << arguments.messageSize << ' '
                 << "--testArg1=" << static_cast<int>(static_cast<UsmMemoryPlacement>(arguments.sendType)) << ' '
                 << "--testArg2=" << static_cast<int>(static_cast<UsmMemoryPlacement>(arguments.recvType));

    auto measurements = runLauncher(arguments.mpiLauncher, arguments.numberOfRanks, "mpi_workload_l0", workloadArgs.str(), arguments.iterations);

    if (measurements.size() != arguments.iterations) {
        return TestResult::Error;
    }
    for (auto measurement : measurements) {
        statistics.pushValue(measurement, typeSelector.getUnit(), typeSelector.getType());
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<MpiReduce> registerTestCase(run, Api::L0);
