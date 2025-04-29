/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct SubmitBarrierArguments : TestCaseArgumentContainer {
    BooleanArgument useHostTasks;
    BooleanArgument inOrderQueue;
    BooleanArgument useEvents;
    BooleanArgument submitBarrier;
    BooleanArgument getLastEvent;

    SubmitBarrierArguments()
        : useHostTasks(*this, "UseHostTasks", "Submit SYCL host task after kernel enqueue"),
          inOrderQueue(*this, "Ioq", "Create the queue with the in_order property"),
          useEvents(*this, "UseEvents", "Use events when enqueuing kernels. When false, SYCL will use eventless enqueue functions."),
          submitBarrier(*this, "SubmitBarrier", "Whether to measure ext_oneapi_submit_barrier time"),
          getLastEvent(*this, "GetLastEvent", "Whether to measure ext_oneapi_get_last_event time") {}
};

struct SubmitBarrier : TestCase<SubmitBarrierArguments> {
    using TestCase<SubmitBarrierArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "SubmitBarrier";
    }

    std::string getHelp() const override {
        return "measures time spent in submitting a barrier to a SYCL (or SYCL-like) queue on CPU and getting last event.";
    }
};
