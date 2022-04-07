/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/test_case/test_case_interface.h"

#include <map>
#include <memory>
#include <string>

struct LexicographicalLess {
    bool operator()(const std::string &left, const std::string &right) const {
        return left < right;
    }
};

struct TestMap : std::map<std::string, std::unique_ptr<TestCaseInterface>, LexicographicalLess> {
    static TestMap &get() {
        static TestMap testMap = {};
        return testMap;
    }
};
