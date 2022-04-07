/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/l0/utility/error_codes.h"
#include "framework/utility/error.h"

#include <string>

#define ASSERT_ZE_RESULT_SUCCESS(retVal)                                                                                                       \
    {                                                                                                                                          \
        const ze_result_t tempVarForDefine = (retVal);                                                                                         \
        if (tempVarForDefine != ZE_RESULT_SUCCESS) {                                                                                           \
            NON_FATAL_ERROR("ASSERT_ZE_RESULT_SUCCESS", #retVal, std::to_string(tempVarForDefine).c_str(), l0ErrorToString(tempVarForDefine)); \
            return TestResult::Error;                                                                                                          \
        }                                                                                                                                      \
    }

#define EXPECT_ZE_RESULT_SUCCESS(retVal)                                                                                                       \
    {                                                                                                                                          \
        const ze_result_t tempVarForDefine = (retVal);                                                                                         \
        if (tempVarForDefine != ZE_RESULT_SUCCESS) {                                                                                           \
            NON_FATAL_ERROR("EXPECT_ZE_RESULT_SUCCESS", #retVal, std::to_string(tempVarForDefine).c_str(), l0ErrorToString(tempVarForDefine)); \
        }                                                                                                                                      \
    }

#define ZE_RESULT_SUCCESS_OR_RETURN_VALUE(retVal, value) \
    if ((retVal) != ZE_RESULT_SUCCESS) {                 \
        return (value);                                  \
    }

#define ZE_RESULT_SUCCESS_OR_RETURN_FALSE(retVal) \
    ZE_RESULT_SUCCESS_OR_RETURN_VALUE((retVal), false)

#define ZE_RESULT_SUCCESS_OR_RETURN_ERROR(retVal) \
    ZE_RESULT_SUCCESS_OR_RETURN_VALUE((retVal), TestResult::Error)

#define ZE_RESULT_SUCCESS_OR_RETURN(retVal)                                    \
    {                                                                          \
        const auto tempVarForDefine = (retVal);                                \
        ZE_RESULT_SUCCESS_OR_RETURN_VALUE(tempVarForDefine, tempVarForDefine); \
    }

#define ZE_RESULT_SUCCESS_OR_ERROR(retVal)                                                                                         \
    {                                                                                                                              \
        const auto tempVarForDefine = (retVal);                                                                                    \
        if (tempVarForDefine != ZE_RESULT_SUCCESS) {                                                                               \
            const auto _message = std::string("Fatal LevelZero error occurred, retVal=") + std::to_string(tempVarForDefine) + ")"; \
            FATAL_ERROR(_message);                                                                                                 \
        }                                                                                                                          \
    }
