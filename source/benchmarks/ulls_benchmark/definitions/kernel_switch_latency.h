/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/work_item_id_usage_argument.h"
#include "framework/test_case/test_case.h"

#include <sstream>

struct KernelSwitchLatencyArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument kernelCount;
    PositiveIntegerArgument kernelExecutionTime;
    BooleanArgument flushBetweenEnqueues;
    BooleanArgument barrier;
    BooleanArgument hostVisible;

    KernelSwitchLatencyArguments()
        : kernelCount(*this, "kernelCount", "Count of kernels"),
          kernelExecutionTime(*this, "kernelExecutionTime", "Approximately how long a single kernel executes, in us"),
          flushBetweenEnqueues(*this, "flush", "Flush between kernels"),
          barrier(*this, "barrier", "synchronization with barrier instead of events"),
          hostVisible(*this, "hostVisible", "events are with host visible flag") {}
};

struct KernelSwitchLatency : TestCase<KernelSwitchLatencyArguments> {
    using TestCase<KernelSwitchLatencyArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "KernelSwitchLatency";
    }

    std::string getHelp() const override {
        return "measures time from end of one kernel till start of next kernel";
    }
};

inline auto selectKernel(WorkItemIdUsage usedIds, const char *extension) {
    std::ostringstream result{};
    switch (usedIds) {
    case WorkItemIdUsage::None:
        result << "ulls_benchmark_write_one";
        break;
    case WorkItemIdUsage::Global:
        result << "ulls_benchmark_write_one_global_ids";
        break;
    case WorkItemIdUsage::Local:
        result << "ulls_benchmark_write_one_local_ids";
        break;
    case WorkItemIdUsage::AtomicPerWorkgroup:
        result << "ulls_benchmark_write_one_atomic_per_workgroup";
        break;
    default:
        FATAL_ERROR("Unknown work item id usage");
    }
    result << '.' << extension;
    return result.str();
}
