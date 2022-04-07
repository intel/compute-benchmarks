/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/utility/command_line_argument.h"

#include <memory>
#include <string>

struct ArgumentContainer;

struct TestCaseInterface {
    virtual ~TestCaseInterface() = default;
    virtual bool runFromCommandLine(CommandLineArguments &commandLineArguments) = 0;
    virtual bool isApiImplemented(Api api) const = 0;
    virtual std::vector<Api> getApisWithImplementation() const = 0;
    virtual std::unique_ptr<ArgumentContainer> getArguments() const = 0;
    virtual std::string getHelp() const = 0;
    virtual std::string getHelpParameters() const = 0;
    virtual std::string getTestCaseName() const = 0;
};
