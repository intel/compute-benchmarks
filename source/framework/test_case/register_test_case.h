/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/test_case/test_case.h"
#include "framework/test_map.h"
#include "framework/utility/string_utils.h"

template <typename TestCase>
struct RegisterTestCaseImplementation {
    explicit RegisterTestCaseImplementation(typename TestCase::BenchmarkImplementation::Function function, Api api, bool requiresIntelExtensions = false) {
        auto &implementation = TestCase::implementations[(int)api];
        implementation.function = function;
        implementation.requiresIntelExtensions = requiresIntelExtensions;
    }
};

template <typename ConcreteTestCase>
struct RegisterTestCase {
    explicit RegisterTestCase() {
        using ExpectedBaseClass = TestCase<typename ConcreteTestCase::ArgumentContainerT>;
        static_assert(std::is_base_of_v<ExpectedBaseClass, ConcreteTestCase>, "ConcreteTestCase should derive from TestCase");
        TestMap &testMap = TestMap::get();

        auto testCase = std::unique_ptr<TestCaseInterface>(new ConcreteTestCase());
        DEVELOPER_WARNING_IF(testMap.find(testCase->getTestCaseName()) != testMap.end(), "Multiple tests have a name \"", testCase->getTestCaseName(), "\"");
        DEVELOPER_WARNING_IF(testCase->getTestCaseName().empty(), "Test case with empty name detected");
        DEVELOPER_WARNING_IF(containsIllegalCharacters(testCase->getTestCaseName(), " -:='\""), "Test case name \"", testCase->getTestCaseName(), "\" contains illegal characters");

        TestMap::get()[testCase->getTestCaseName()] = std::move(testCase);
    }
};
