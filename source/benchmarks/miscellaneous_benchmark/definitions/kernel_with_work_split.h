/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/work_item_id_usage_argument.h"
#include "framework/test_case/test_case.h"

#include <sstream>

struct KernelWithWorkArgumentsSplit : TestCaseArgumentContainer {
    WorkItemIdUsageArgument usedIds;
    PositiveIntegerArgument workgroupCount;
    PositiveIntegerArgument workgroupSize;
    PositiveIntegerArgument splitSize;
    BooleanArgument useEvents;

    KernelWithWorkArgumentsSplit()
        : usedIds(*this, "usedIds", "Which of the get_global_id() and get_local_id() calls will be used in the kernel"),
          workgroupCount(*this, "wgc", "Workgroup count"),
          workgroupSize(*this, "wgs", "Workgroup size (aka local work size)"),
          splitSize(*this, "split", "How many times kernel is split"),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()) {}
};

struct KernelWithWorkSplit : TestCase<KernelWithWorkArgumentsSplit> {
    using TestCase<KernelWithWorkArgumentsSplit>::TestCase;

    std::string getTestCaseName() const override {
        return "KernelWithWork";
    }

    std::string getHelp() const override {
        return "measures time required to run a GPU kernel which assigns constant values to "
               "elements of a buffer. Each thread assigns one value. Benchmark checks the impact of kernel split.";
    }
};

inline auto selectKernel(WorkItemIdUsage usedIds, const char *extension) {
    std::ostringstream result{};
    switch (usedIds) {
    case WorkItemIdUsage::None:
        result << "benchmark_write_one";
        break;
    case WorkItemIdUsage::Global:
        result << "benchmark_write_one_global_ids";
        break;
    case WorkItemIdUsage::Local:
        result << "benchmark_write_one_local_ids";
        break;
    case WorkItemIdUsage::AtomicPerWorkgroup:
        result << "benchmark_write_one_atomic_per_workgroup";
        break;
    default:
        FATAL_ERROR("Unknown work item id usage");
    }
    result << '.' << extension;
    return result.str();
}
