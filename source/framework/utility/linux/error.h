/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/utility/error.h"

#include <sstream>
#include <string.h>
#include <string>

static std::string getErrorFromErrno() {
    std::ostringstream result{};
    result << "errno=" << errno;
    result << " (" << strerror(errno) << ")";
    return result.str();
}

#define FATAL_ERROR_IF_SYS_CALL_FAILED(call, ...) FATAL_ERROR_IF(call < 0, __VA_ARGS__, ", ", getErrorFromErrno());
