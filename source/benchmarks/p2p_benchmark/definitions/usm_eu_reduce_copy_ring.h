/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct UsmEUReduceCopyRingArguments : TestCaseArgumentContainer {
    IntegerArgument numDevices;
    ByteSizeArgument size;
    BooleanArgument useEvents;

    UsmEUReduceCopyRingArguments()
        : numDevices(*this, "numDevices", "Number of root devices in the ring. Test is skipped when the system has fewer devices"),
          size(*this, "size", "Size of the message reduced and written to the peer by each device"),
          useEvents(*this, "useEvents", "Report the longest kernel time across devices measured with GPU timestamps. Otherwise report the CPU time from releasing the kernels until all devices complete") {}
};

struct UsmEUReduceCopyRing : TestCase<UsmEUReduceCopyRingArguments> {
    using TestCase<UsmEUReduceCopyRingArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmEUReduceCopyRing";
    }

    std::string getHelp() const override {
        return "connects numDevices root devices in a ring and runs on every device the reduce-copy step of the oneCCL "
               "ring reduce_scatter and allreduce algorithms: out1 = in1 + in2 in local device memory, then out1 is "
               "written by the EU to a buffer on the next device. The kernel is compiled at run time from "
               "p2p_benchmark_reduce_copy.cl, which can be modified next to the binary without rebuilding. All "
               "devices start together, so with 2 devices both directions of the link carry writes at the same time. "
               "Reports aggregate peer write bandwidth, i.e. numDevices * size per iteration.";
    }
};
