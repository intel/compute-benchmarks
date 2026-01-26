/*
 * Copyright (C) 2023-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct MultiProcessImmediateCmdlistSubmissionArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument numberOfProcesses;

    MultiProcessImmediateCmdlistSubmissionArguments()
        : numberOfProcesses(*this, "numberOfProcesses", "total numer of processes") {}
};

struct MultiProcessImmediateCmdlistSubmission : TestCase<MultiProcessImmediateCmdlistSubmissionArguments> {
    using TestCase<MultiProcessImmediateCmdlistSubmissionArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "MultiProcessImmediateCmdlistSubmission";
    }

    std::string getHelp() const override {
        return "measures submission latency of walker command issued from multiple processes to Immediate Command Lists."
               "If 'numberOfProcesses' is greater than engine count, then the excess processes are "
               "assigned to engines one each, in a round-robin method."
               "if engineCount == 1, then all processes are assigned to the engine.";
    }
};
