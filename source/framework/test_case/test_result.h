/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <string>
#include <unordered_map>

#define ASSERT_TEST_RESULT_SUCCESS(retVal)             \
    do {                                               \
        const TestResult tempVarForDefine = (retVal);  \
        if (tempVarForDefine != TestResult::Success) { \
            return tempVarForDefine;                   \
        }                                              \
    } while (0)

enum class TestResult {
    Success,                 // should be returned after a successful run
    Error,                   // an error was returned by the compute API
    DriverFunctionNotFound,  // extension function was not found and test is skipped
    DeviceNotCapable,        // device does not support some functionality needed in test (e.g. compression)
    ApiNotCapable,           // current graphics API doesn't have support for given parameters
    KernelNotFound,          // binary kernel was not found in working directory
    SkippedApi,              // selected API should not be run, because of user has disabled it with a command-line argument
    UnsupportedApi,          // selected API should not be run, because it is not built in current binary
    NoImplementation,        // Test is not implemented in current API
    IntelExtensionsRequired, // Intel extensions are required, but they are disabled
    InvalidArgs,             // Invalid arguments specific to the test case were supplied
    Nooped,                  // Test was nooped, only print its name
    FilteredOut,             // Test was skipped because of passed argFilter
    VerificationFail,        // Results where incorrect
    KernelBuildError         // Kernel could not be compiled
};

struct TestResultHelper {
    struct TestResultInfo {
        const std::string stringMessage;
        const bool printInSingleTestMode;
        const bool printInAllTestsMode;
        const bool wasTestSkipped;
        const bool printInPrintAllResultsMode;
    };

    static const TestResultInfo &getTestResultInfo(TestResult testResult);
};
