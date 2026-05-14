/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct AppendKernelWithMappedTimestampEventArguments : TestCaseArgumentContainer {
    BooleanArgument useMappedTimestampEvent;

    AppendKernelWithMappedTimestampEventArguments()
        : useMappedTimestampEvent(*this, "useMappedTimestamp",
                                  "Use ZE_EVENT_POOL_FLAG_KERNEL_MAPPED_TIMESTAMP instead of "
                                  "ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP.") {}
};

struct AppendKernelWithMappedTimestampEvent : TestCase<AppendKernelWithMappedTimestampEventArguments> {
    using TestCase<AppendKernelWithMappedTimestampEventArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "AppendKernelWithMappedTimestampEvent";
    }

    std::string getHelp() const override {
        return "Measures CPU time of zeCommandListAppendLaunchKernel with timestamp event.";
    }
};
