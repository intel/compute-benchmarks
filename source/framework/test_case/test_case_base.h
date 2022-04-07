/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/enum/api.h"
#include "framework/test_case/test_case_interface.h"

struct TestCaseArgumentContainer;

// This class implements test-agnostic functionality of the TestCase class. All methods, which do not require
// a concrete TestCaseArgument class for a specific test should be placed in this class as a protected method.
class TestCaseBase : public TestCaseInterface {
  protected:
    static bool parseArguments(TestCaseArgumentContainer &arguments, CommandLineArguments &commandLineArguments);
    std::vector<Api> getApisWithImplementation() const override;
    std::string getTestCaseNameWithConfig(const TestCaseArgumentContainer &arguments, bool commandLine) const;

    // Filters
    bool matchesWithTestFilter() const;
    bool matchesWithArgFilter(const ArgumentContainer &arguments) const;

    // Warnings
    void printTestMapWarning() const;
    void printTestCaseNameLengthWarning(const std::string &testCaseNameWithConfig) const;
};
