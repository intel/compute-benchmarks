/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct KernelSubmitEventRecordQueryArguments : TestCaseArgumentContainer {
    Uint32Argument kernelWGCount;
    Uint32Argument kernelWGSize;
    Uint32Argument eventQueryIterations;
    BooleanArgument useProfiling;

    KernelSubmitEventRecordQueryArguments() : kernelWGCount(*this, "KernelWGCount", "Number of workgroups."),
                                              kernelWGSize(*this, "KernelWGSize", "Size of each workgroup."),
                                              eventQueryIterations(*this, "EventQueryIterations", "Number of event status queries after event record."),
                                              useProfiling(*this, "Profiling", "Create queues with profiling enabled.") {}
};

struct KernelSubmitEventRecordQuery : TestCase<KernelSubmitEventRecordQueryArguments> {
    using TestCase<KernelSubmitEventRecordQueryArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "KernelSubmitEventRecordQuery";
    }

    std::string getHelp() const override {
        return "Measures the time to query status of completed event, multiple times.";
    }
};
