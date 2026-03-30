/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/configuration.h"
#include "framework/test_case/test_case_statistics.h"

#include <algorithm>
#include <gtest/gtest.h>
#include <iostream>
#include <sstream>

class AllTestsGtestListener : public ::testing::EmptyTestEventListener {
    struct ErrorInfo {
        ErrorInfo() : name(), errorMessage() {}
        std::ostringstream name;
        std::ostringstream errorMessage;
    } currentTestCaseErrorInfo{};

    int totalTests = 0;
    int completedTests = 0;
    int lastProgressLen = 0;

    bool showProgress() const {
        return !Configuration::get().noHeaders &&
               !Configuration::get().noProgressBar &&
               !Configuration::get().interactivePrints &&
               Configuration::get().printType != Configuration::PrintType::Csv;
    }

    void printProgress(const std::string &currentTestName = "") {
        if (!showProgress()) {
            return;
        }

        constexpr int barWidth = 40;
        const float ratio = totalTests > 0 ? static_cast<float>(completedTests) / totalTests : 0.0f;
        const int filled = static_cast<int>(barWidth * ratio);

        std::ostringstream line;
        line << '[';
        for (int i = 0; i < barWidth; i++) {
            if (i < filled)
                line << '=';
            else if (i == filled && completedTests < totalTests)
                line << '>';
            else
                line << ' ';
        }
        line << "] " << completedTests << '/' << totalTests;
        if (!currentTestName.empty()) {
            line << "  " << currentTestName;
        }

        const std::string lineStr = line.str();
        const int padding = std::max(0, lastProgressLen - static_cast<int>(lineStr.size()));
        lastProgressLen = static_cast<int>(lineStr.size());
        std::cout << '\r' << lineStr << std::string(padding, ' ') << std::flush;
    }

    void clearProgress() {
        if (!showProgress()) {
            return;
        }
        std::cout << '\r' << std::string(lastProgressLen, ' ') << '\r' << std::flush;
        lastProgressLen = 0;
    }

    void OnTestProgramStart(const ::testing::UnitTest &unitTest) override {
        totalTests = unitTest.total_test_count();
        if (!Configuration::get().noHeaders && Configuration::get().printType != Configuration::PrintType::Csv) {
            std::cout << "Running " << Configuration::get().iterations << " iterations of each benchmark\n\n";
        }
        // For CSV, print the header immediately (no alignment needed).
        // For fixed-width modes, the header is deferred to OnTestProgramEnd so the
        // TestCase column width can be sized to the longest name seen across all tests.
        if (!Configuration::get().noColumnNames && Configuration::get().printType == Configuration::PrintType::Csv) {
            TestCaseStatistics::printStatisticsHeader(Configuration::get().printType, 0);
        }
        printProgress();
    }
    void OnTestProgramEnd([[maybe_unused]] const ::testing::UnitTest &unitTest) override {
        clearProgress();
        TestCaseStatistics::flushBufferedResults(Configuration::get().printType);
        dumpErrors();
    }

    void OnTestStart(const ::testing::TestInfo &testCase) override {
        currentTestCaseErrorInfo = {};
        printProgress(testCase.test_suite_name());
    }
    void OnTestPartResult(const ::testing::TestPartResult &testPartResult) override {
        if (testPartResult.failed()) {
            currentTestCaseErrorInfo.errorMessage << testPartResult.message() << "\n";
        }
    }
    void OnTestEnd(const ::testing::TestInfo &testCase) override {
        completedTests++;
        printProgress();
        if (testCase.result()->Failed()) {
            currentTestCaseErrorInfo.name << testCase.test_case_name() << "." << testCase.name();
            errorInfos.push_back(std::move(currentTestCaseErrorInfo));
        }
        if (Configuration::get().dumpErrorsImmediately) {
            dumpErrors();
        }
    }

    void dumpErrors() {
        if (errorInfos.size() > 0) {
            std::cout << "\n";
            for (const auto &errorInfo : errorInfos) {
                std::cout << "[  FAILED  ] " << errorInfo.name.str() << '\n'
                          << errorInfo.errorMessage.str() << '\n';
            }
        }
        errorInfos.clear();
    }

    std::vector<ErrorInfo> errorInfos = {};
};

class SingleTestGtestListener : public ::testing::EmptyTestEventListener {
    void OnTestPartResult(const ::testing::TestPartResult &testPartResult) override {
        if (testPartResult.failed()) {
            std::cout << "\n"
                      << testPartResult.message()
                      << "\n";
        }
    }
};

template <typename ListenerType>
void replaceGtestListener() {
    auto &listeners = ::testing::UnitTest::GetInstance()->listeners();
    delete listeners.Release(listeners.default_result_printer());
    listeners.Append(new ListenerType());
}
