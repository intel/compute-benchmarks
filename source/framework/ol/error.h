/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/utility/error.h"

#include <OffloadAPI.h>
#include <sstream>
#include <string>

#define ASSERT_OL_RESULT_SUCCESS(retVal)                                                               \
    do {                                                                                               \
        const ol_result_t tempVarForDefine = (retVal);                                                 \
        if (tempVarForDefine != OL_SUCCESS) {                                                          \
            std::ostringstream ss;                                                                     \
            ss << static_cast<int>(tempVarForDefine->Code) << ": "                                     \
               << (tempVarForDefine->Details ? tempVarForDefine->Details : "(no details)");            \
            const std::string errorCode = std::to_string(static_cast<int>(tempVarForDefine->Code));    \
            NON_FATAL_ERROR("ASSERT_OL_RESULT_SUCCESS", #retVal, errorCode.c_str(), ss.str().c_str()); \
            return TestResult::Error;                                                                  \
        }                                                                                              \
    } while (0)

#define EXPECT_OL_RESULT_SUCCESS(retVal)                                                               \
    do {                                                                                               \
        const ol_result_t tempVarForDefine = (retVal);                                                 \
        if (tempVarForDefine != OL_SUCCESS) {                                                          \
            std::ostringstream ss;                                                                     \
            ss << static_cast<int>(tempVarForDefine->Code) << ": "                                     \
               << (tempVarForDefine->Details ? tempVarForDefine->Details : "(no details)");            \
            const std::string errorCode = std::to_string(static_cast<int>(tempVarForDefine->Code));    \
            NON_FATAL_ERROR("EXPECT_OL_RESULT_SUCCESS", #retVal, errorCode.c_str(), ss.str().c_str()); \
        }                                                                                              \
    } while (0)

#define OL_RESULT_SUCCESS_OR_RETURN_VALUE(retVal, value) \
    do {                                                 \
        const ol_result_t tempVarForDefine = (retVal);   \
        if (tempVarForDefine != OL_SUCCESS) {            \
            return (value);                              \
        }                                                \
    } while (0)

#define OL_RESULT_SUCCESS_OR_ERROR(retVal)                                                          \
    do {                                                                                            \
        const ol_result_t tempVarForDefine = (retVal);                                              \
        if (tempVarForDefine != OL_SUCCESS) {                                                       \
            FATAL_ERROR("Fatal OL error occurred, code=", static_cast<int>(tempVarForDefine->Code), \
                        ", details=",                                                               \
                        tempVarForDefine->Details ? tempVarForDefine->Details : "(no details)");    \
        }                                                                                           \
    } while (0)
