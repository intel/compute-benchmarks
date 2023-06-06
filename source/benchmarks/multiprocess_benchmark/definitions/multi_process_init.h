/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct MultiProcessInitArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument numberOfProcesses;
    IntegerArgument initFlag;

    MultiProcessInitArguments()
        : numberOfProcesses(*this, "numberOfProcesses", "Total number of processes"),
          initFlag(*this, "initFlag", "Initialization flag. For Level Zero: 0 - default, 1 - ZE_INIT_FLAG_GPU_ONLY, 2 - ZE_INIT_FLAG_VPU_ONLY") {
    }
};

struct MultiProcessInit : TestCase<MultiProcessInitArguments> {
    using TestCase<MultiProcessInitArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "MultiProcessInit";
    }

    std::string getHelp() const override {
        return "Measures the initialization overhead in a multi-process application."
               "For Level Zero we only measure the first invocation of zeInit() per process execution.";
    }
};
