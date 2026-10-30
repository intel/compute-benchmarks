/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/enum/api.h"
#include "framework/test_case/test_case_interface.h"

#include <optional>

struct TestCaseArgumentContainer;

class TestCaseBase : public TestCaseInterface {
  public:
    virtual bool hasFixedIterationCount() const { return false; }
    virtual std::optional<size_t> getDefaultIterations() const { return std::nullopt; }
    virtual std::optional<size_t> getDefaultWarmupIterations() const { return std::nullopt; }

  protected:
    static bool parseArguments(TestCaseArgumentContainer &arguments, CommandLineArguments &commandLineArguments);
    std::vector<Api> getApisWithImplementation() const override;
    std::string getTestCaseNameWithConfig(const TestCaseArgumentContainer &arguments, bool commandLine) const;

    size_t getIterationCount() const;
    size_t getWarmupIterationCount() const;

    bool matchesWithTestFilter() const;
    bool matchesWithArgFilter(const ArgumentContainer &arguments) const;
    bool needsToBeFilteredDueToLimitedTargets() const;

    void printTestMapWarning() const;
};
