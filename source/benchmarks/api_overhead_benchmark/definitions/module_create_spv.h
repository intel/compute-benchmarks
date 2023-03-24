/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/string_argument.h"
#include "framework/test_case/test_case.h"

struct ModuleCreateSpvArguments : TestCaseArgumentContainer {
    StringArgument kernelName;

    ModuleCreateSpvArguments()
        : kernelName(*this, "kernelName", "Path to Kernel .spv file") {}
};

struct ModuleCreateSpv : TestCase<ModuleCreateSpvArguments> {
    using TestCase<ModuleCreateSpvArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "ModuleCreateSpv";
    }

    std::string getHelp() const override {
        return "measures time spent in zeModuleCreate for .spv kernel on CPU.";
    }
};
