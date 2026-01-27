/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/utility/error.h"

#include <sstream>
#include <string>
#include <vector>

void assignAndValidate(std::string &destination, const std::string &source, const char *message) {
    if (destination.empty()) {
        destination = source;
    } else {
        FATAL_ERROR_IF(destination != source, "String mismatch detected during ", message);
    }
}

std::pair<std::string, std::string> splitToTwoTokens(const std::string &line) {
    std::vector<std::string> result = {};
    std::istringstream stringStream(line);

    const size_t pos = line.find_first_of(';');
    FATAL_ERROR_IF(pos == std::string::npos, "Expected ';' delimetr in line");
    const std::string token0 = line.substr(0, pos);
    std::string token1 = line.substr(pos + 1);
    FATAL_ERROR_IF(token1.find(';') != std::string::npos, "Too many ';' delimeters in line");

    while (token1.size() > 0 && token1.back() == '\r') {
        token1.pop_back();
    }
    return {token0, token1};
}

bool hasAnyApi(const TestCase &testCase, const std::vector<Api> &apis) {
    for (Api api : apis) {
        if (testCase.apis.count(api) > 0) {
            return true;
        }
    }
    return false;
}
