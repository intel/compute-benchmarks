/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_case_argument_container.h"

#include "framework/argument/abstract/argument.h"

#include <sstream>

std::string TestCaseArgumentContainer::getCurrentConfig(bool commandLine) const {
    std::ostringstream result;
    for (auto i = 0u; i < arguments.size(); i++) {
        if (commandLine) {
            result << "--";
        }
        result << arguments[i]->toString();
        if (i != arguments.size() - 1) {
            result << " ";
        }
    }
    return result.str();
}

bool TestCaseArgumentContainer::validateArguments() const {
    if (!ArgumentContainer::validateArguments()) {
        return false;
    }

    if (!validateApi(api)) {
        return false;
    }

    if (iterations <= 0) {
        return false;
    }

    return true;
}
