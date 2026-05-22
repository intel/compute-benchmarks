/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/test_case/test_result.h"
#include "framework/utility/timer.h"

struct Decoder2GraphArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument numTokens;
    BooleanArgument useGraphs;
    BooleanArgument emulateGraphs;
    BooleanArgument useHostTasks;
    BooleanArgument useNativeRecording;

    Decoder2GraphArguments()
        : numTokens(*this, "NumTokens", "Number of tokens"),
          useGraphs(*this, "UseGraphs", "Use graphs (1) or execute eagerly (0)"),
          emulateGraphs(*this, "EmulateGraphs", "Emulate graph execution (1) or not (0) for L0 backend. Must be 0 for other backends"),
          useHostTasks(*this, "UseHostTasks", "Use host task for single graph execution per token (1) or no host tasks for single graph per layer (0)"),
          useNativeRecording(*this, "UseNativeRecording", "Enables property::graph::enable_native_recording on the command graph (only valid for SYCL). With UseHostTasks=1, uses zeCommandListAppendHostFunction instead of SYCL host_task, which is unsupported in native recording mode.") {}
};

struct Decoder2Graph : TestCase<Decoder2GraphArguments> {
    using TestCase<Decoder2GraphArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "Decoder2Graph";
    }

    std::string getHelp() const override {
        return "LLM decode proxy benchmark to simulate graph execution with host CPU coordination.";
    }
};
