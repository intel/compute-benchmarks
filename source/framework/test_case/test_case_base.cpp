/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_case_base.h"

#include "framework/benchmark_info.h"
#include "framework/configuration.h"
#include "framework/test_case/test_case_argument_container.h"

bool TestCaseBase::parseArguments(TestCaseArgumentContainer &arguments, CommandLineArguments &commandLineArguments) {
    arguments.isSingleTestMode = true;
    for (auto &commandLineArgument : commandLineArguments) {
        if (!arguments.parseArgument(commandLineArgument)) {
            return false;
        }
    }
    return true;
}

std::vector<Api> TestCaseBase::getApisWithImplementation() const {
    std::vector<Api> apis = {};
    for (int apiIndex = static_cast<int>(Api::FIRST); apiIndex <= static_cast<int>(Api::LAST); apiIndex++) {
        const Api api = static_cast<Api>(apiIndex);
        if (isApiImplemented(api)) {
            apis.push_back(api);
        }
    }
    return apis;
}

std::string TestCaseBase::getTestCaseNameWithConfig(const TestCaseArgumentContainer &arguments, bool commandLine) const {
    std::ostringstream result{};

    if (commandLine) {
        result << "--test=" << getTestCaseName() << " --api=" << std::to_string(arguments.api);
    } else {
        result << getTestCaseName() << "(api=" << std::to_string(arguments.api);
    }

    const auto config = arguments.getCurrentConfig(commandLine);
    if (config.size() > 0) {
        result << " " << config;
    }

    if (!commandLine) {
        result << ")";
    }

    return result.str();
}

bool TestCaseBase::matchesWithTestFilter() const {
    for (const std::string &testFilter : Configuration::get().testFilter.get()) {
        const auto testCaseName = getTestCaseName();
        const auto [filter, isFilterNegated] = handleFilterNegation(testFilter);
        const auto requirementMet = (testCaseName == filter) != isFilterNegated;
        if (!requirementMet) {
            return false;
        }
    }
    return true;
}

bool TestCaseBase::matchesWithArgFilter(const ArgumentContainer &arguments) const {
    for (const std::string &argFilter : Configuration::get().argFilter.get()) {
        const std::vector<Argument *> &args = arguments.getArguments();
        const auto [filter, isFilterNegated] = handleFilterNegation(argFilter);
        const auto matches = [&filter = filter](Argument *arg) { return (arg->toString() == filter); };
        const bool requirementMet = isFilterNegated
                                        ? std::none_of(args.begin(), args.end(), matches)
                                        : std::any_of(args.begin(), args.end(), matches);
        if (!requirementMet) {
            return false;
        }
    }
    return true;
}

bool TestCaseBase::needsToBeFilteredDueToLimitedTargets() const {
    const auto *test_info = ::testing::UnitTest::GetInstance()->current_test_info();
    const auto full_suite_name = std::string(test_info->test_suite_name());
    if (full_suite_name.find("LIMITED/") != std::string::npos) {
        const auto pos = full_suite_name.find('/');
        if (pos != std::string::npos) {
            const auto instantiate_name = full_suite_name.substr(0, pos);
            const auto test_class_name = full_suite_name.substr(pos + 1);
            if (instantiate_name != test_class_name) {
                return !Configuration::get().allowLimitedTests;
            }
        }
    }
    return Configuration::get().allowLimitedTests;
}

void TestCaseBase::printTestMapWarning() const {
    static bool printed = false;
    if (printed) {
        return;
    }
    printed = true;

    std::cerr << "WARNING: \"" << getTestCaseName() << "\" is not added to the test map. This is an issue in the benchmark causing single-test mode to not work.\n";
}

void TestCaseBase::printTestCaseNameLengthWarning(const std::string &testCaseNameWithConfig) const {
    const static size_t columnWidth = BenchmarkInfo::get().getTestCaseNameColumnWidth();
    static size_t maxWidth = columnWidth;

    const size_t currentWidth = testCaseNameWithConfig.length();
    if (currentWidth > maxWidth && Configuration::get().verbose) {
        maxWidth = currentWidth;
        std::cerr << "WARNING: current TestCase column width of " << columnWidth << " is too small. Consider changing it to " << maxWidth << ". "
                  << "This is an issue in the benchmark which may cause the output to appear weird, but does not break any functionality.\n";
    }
}
