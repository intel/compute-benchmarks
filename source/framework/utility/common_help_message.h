/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <string>

struct CommonHelpMessage {
    static std::string errorIgnoredCommandLineArgs();
    static std::string errorUnsetArguments();

    static std::string compression(const char *target);
    static std::string forceBlitter();
    static std::string useEvents();
    static std::string measuredCommandsCount();
    static std::string atomicDataType();
    static std::string hostptrBufferReuse();
    static std::string writeOperation();
};
