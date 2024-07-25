/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/utility/error.h"

#include <sstream>
#include <string>
#include <ur_print.hpp>

#define ASSERT_UR_RESULT_SUCCESS(retVal)                                                                                      \
    {                                                                                                                         \
        const ur_result_t tempVarForDefine = (retVal);                                                                        \
        if (tempVarForDefine != UR_RESULT_SUCCESS) {                                                                          \
            std::ostringstream ss;                                                                                            \
            ss << tempVarForDefine;                                                                                           \
            NON_FATAL_ERROR("ASSERT_UR_RESULT_SUCCESS", #retVal, std::to_string(tempVarForDefine).c_str(), ss.str().c_str()); \
            return TestResult::Error;                                                                                         \
        }                                                                                                                     \
    }

#define EXPECT_UR_RESULT_SUCCESS(retVal)                                                                                      \
    {                                                                                                                         \
        const ur_result_t tempVarForDefine = (retVal);                                                                        \
        if (tempVarForDefine != UR_RESULT_SUCCESS) {                                                                          \
            std::ostringstream ss;                                                                                            \
            ss << tempVarForDefine;                                                                                           \
            NON_FATAL_ERROR("EXPECT_UR_RESULT_SUCCESS", #retVal, std::to_string(tempVarForDefine).c_str(), ss.str().c_str()); \
        }                                                                                                                     \
    }

#define UR_RESULT_SUCCESS_OR_RETURN_VALUE(retVal, value) \
    if ((retVal) != UR_RESULT_SUCCESS) {                 \
        return (value);                                  \
    }

#define UR_RESULT_SUCCESS_OR_RETURN_FALSE(retVal) \
    UR_RESULT_SUCCESS_OR_RETURN_VALUE((retVal), false)

#define UR_RESULT_SUCCESS_OR_RETURN_ERROR(retVal) \
    UR_RESULT_SUCCESS_OR_RETURN_VALUE((retVal), TestResult::Error)

#define UR_RESULT_SUCCESS_OR_RETURN(retVal)                                    \
    {                                                                          \
        const auto tempVarForDefine = (retVal);                                \
        UR_RESULT_SUCCESS_OR_RETURN_VALUE(tempVarForDefine, tempVarForDefine); \
    }

#define UR_RESULT_SUCCESS_OR_ERROR(retVal)                                                                                  \
    {                                                                                                                       \
        const auto tempVarForDefine = (retVal);                                                                             \
        if (tempVarForDefine != UR_RESULT_SUCCESS) {                                                                        \
            const auto _message = std::string("Fatal UR error occurred, retVal=") + std::to_string(tempVarForDefine) + ")"; \
            FATAL_ERROR(_message);                                                                                          \
        }                                                                                                                   \
    }
