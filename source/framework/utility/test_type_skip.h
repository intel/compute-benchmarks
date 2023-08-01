/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/enum/test_type.h"

inline bool isTestSkipped(bool extendedConf, TestType testType) {
    if ((extendedConf && testType == TestType::Regular) ||
        (!extendedConf && testType == TestType::Extended) ||
        (!extendedConf && testType == TestType::ReducedSizeCAL)) {
        return true;
    }
    return false;
}
