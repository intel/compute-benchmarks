/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/test_case/test_case.h"

struct SlmSwitchLatencyArguments : TestCaseArgumentContainer {
    ByteSizeArgument slmPerWkgKernel1;
    ByteSizeArgument slmPerWkgKernel2;
    ByteSizeArgument wgs;

    SlmSwitchLatencyArguments()
        : slmPerWkgKernel1(*this, "firstSlmSize", "Size of the shared local memory per thread group. First kernel."),
          slmPerWkgKernel2(*this, "secondSlmSize", "Size of the shared local memory per thread group. Second kernel."),
          wgs(*this, "wgs", "Size of the work group.") {}
};

struct SlmSwitchLatency : TestCase<SlmSwitchLatencyArguments> {
    using TestCase<SlmSwitchLatencyArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "SlmSwitchLatency";
    }

    std::string getHelp() const override {
        return "Enqueues 2 kernels with different SLM size. Measures switch time between these kernels.";
    }
};
