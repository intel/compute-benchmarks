/*
 * Copyright (C) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/enum/mpi_test.h"
#include "framework/test_case/register_test_case.h"

#include "definitions/overlap.h"
#include "utility/mpi_helper.h"

#include <gtest/gtest.h>
#include <sstream>
#include <unistd.h>
#include <vector>

static TestResult run(const MpiOverlapArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Percentage, MeasurementType::Cpu);

    std::stringstream workloadArgs;
    workloadArgs << "--testType=" << static_cast<int>(MpiTestType::Overlap) << ' '
                 << "--testArg0=" << arguments.messageSize << ' '
                 << "--testArg1=" << static_cast<int>(static_cast<UsmMemoryPlacement>(arguments.sendType)) << ' '
                 << "--testArg2=" << static_cast<int>(static_cast<UsmMemoryPlacement>(arguments.recvType)) << ' '
                 << "--testArg3=" << arguments.gpuCompute << ' '
                 << "--testArg4=" << arguments.numMpiTestCalls;

    auto measurements = runLauncher(arguments.mpiLauncher, 2, "mpi_workload_l0", workloadArgs.str(), arguments.iterations);

    if (measurements.size() != arguments.iterations) {
        return TestResult::Error;
    }
    for (auto measurement : measurements) {
        const double overlapPercentage = measurement.count() / 1e5;
        statistics.pushPercentage(overlapPercentage, typeSelector.getUnit(), typeSelector.getType());
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<MpiOverlap> registerTestCase(run, Api::L0);
