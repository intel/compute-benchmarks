/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/enum/api.h"
#include "framework/test_case/test_case_interface.h"

struct TestCaseArgumentContainer;

class TestCaseBase : public TestCaseInterface {
  protected:
    static bool parseArguments(TestCaseArgumentContainer &arguments, CommandLineArguments &commandLineArguments);
    std::vector<Api> getApisWithImplementation() const override;
    std::string getTestCaseNameWithConfig(const TestCaseArgumentContainer &arguments, bool commandLine) const;

    bool matchesWithTestFilter() const;
    bool matchesWithArgFilter(const ArgumentContainer &arguments) const;
    bool needsToBeFilteredDueToLimitedTargets() const;

    void printTestMapWarning() const;
};
