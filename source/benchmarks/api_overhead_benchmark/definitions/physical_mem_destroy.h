/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct PhysicalMemDestroyArguments : TestCaseArgumentContainer {
    PhysicalMemDestroyArguments() {}
};

struct PhysicalMemDestroy : TestCase<PhysicalMemDestroyArguments> {
    using TestCase<PhysicalMemDestroyArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "PhysicalMemDestroy";
    }

    std::string getHelp() const override {
        return "measures time spent in zePhysicalMemDestroy on CPU.";
    }
};
