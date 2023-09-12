/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/mpi_statistics_type_argument.h"
#include "framework/test_case/test_case.h"

struct MpiStartupArguments : TestCaseArgumentContainer {
    StringArgument mpiLauncher;
    MpiStatisticsTypeArgument statsType;

    PositiveIntegerArgument numberOfRanks;
    BooleanArgument initMpiFirst;
    IntegerArgument initFlag;

    MpiStartupArguments()
        : mpiLauncher(*this, "mpiLauncher", "MPI launcher (e.g. mpirun --hostfile /path/to/hosts"),
          statsType(*this, "statsType", "MPI statistics type (Avg/Max/Min)"),
          numberOfRanks(*this, "numberOfRanks", "Total number of MPI ranks"),
          initMpiFirst(*this, "initMpiFirst", "Initialize MPI before L0"),
          initFlag(*this, "initFlag", "Initialization flag. For Level Zero: 0 - default, 1 - ZE_INIT_FLAG_GPU_ONLY, 2 - ZE_INIT_FLAG_VPU_ONLY") {
        mpiLauncher = "mpirun";
        statsType = MpiStatisticsType::Avg;
        numberOfRanks = 1;
        initMpiFirst = true;
        initFlag = 0;
    }
};

struct MpiStartup : public TestCase<MpiStartupArguments> {
    using TestCase<MpiStartupArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "MpiStartup";
    }

    std::string getHelp() const override {
        return "Measures the initialization overhead in an MPI+X application.";
    }
};
