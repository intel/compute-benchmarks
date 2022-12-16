/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct MultiProcessImmediateCmdlistSubmissionArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument numberOfProcesses;
    PositiveIntegerArgument processesPerEngine;

    MultiProcessImmediateCmdlistSubmissionArguments()
        : numberOfProcesses(*this, "numberOfProcesses", "total numer of processes"),
          processesPerEngine(*this, "processesPerEngine", "number of processes submitting commands to each engine") {}
};

struct MultiProcessImmediateCmdlistSubmission : TestCase<MultiProcessImmediateCmdlistSubmissionArguments> {
    using TestCase<MultiProcessImmediateCmdlistSubmissionArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "MultiProcessImmediateCmdlistSubmission";
    }

    std::string getHelp() const override {
        return "measures submission latency of walker command issued from multiple processes to Immediate Command Lists."
               "'processesPerEngine' count of processes, submit commands to each engine."
               "If 'numberOfProcesses' is greater than 'processesPerEngine' x engine count, then the excess processes are "
               "assigned to engines one each, in a round-robin method."
               "if engineCount == 1, then all processes are assigned to the engine.";
    }
};
