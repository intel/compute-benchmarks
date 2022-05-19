/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct ResourceReassignArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument queueCount;

    ResourceReassignArguments() : queueCount(*this, "queueCount", "number of different command queues to which submits after stress kernel"){};
};

struct ResourceReassign : TestCase<ResourceReassignArguments> {
    using TestCase<ResourceReassignArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "ResourceReassign";
    }

    std::string getHelp() const override {
        return "Enqueues stress kernel which utilizes majority of GPU's execution units, then enqueues next kernel, measuring its execution time. Shows overhead releated to GPU's resources releasing and assigning.";
    }
};
