/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/ocl/utility/error_codes.h"
#include "framework/utility/error.h"

#include <string>

#define ASSERT_CL_SUCCESS(retVal)                                                                                                        \
    {                                                                                                                                    \
        const cl_int tempVarForDefine = (retVal);                                                                                        \
        if (tempVarForDefine != CL_SUCCESS) {                                                                                            \
            NON_FATAL_ERROR("ASSERT_CL_SUCCESS", #retVal, std::to_string(tempVarForDefine).c_str(), oclErrorToString(tempVarForDefine)); \
            return TestResult::Error;                                                                                                    \
        }                                                                                                                                \
    }

#define ASSERT_CL_COMPILATION_SUCCESS(retVal)                                                                                            \
    {                                                                                                                                    \
        const cl_int tempVarForDefine = (retVal);                                                                                        \
        if (tempVarForDefine != CL_SUCCESS) {                                                                                            \
            size_t numBytes = 0;                                                                                                         \
            clGetProgramBuildInfo(program, opencl.device, CL_PROGRAM_BUILD_LOG, 0, NULL, &numBytes);                                     \
            auto buffer = std::make_unique<char[]>(numBytes);                                                                            \
            clGetProgramBuildInfo(program, opencl.device, CL_PROGRAM_BUILD_LOG, numBytes, buffer.get(), &numBytes);                      \
            std::cout << buffer.get() << std::endl;                                                                                      \
            NON_FATAL_ERROR("ASSERT_CL_SUCCESS", #retVal, std::to_string(tempVarForDefine).c_str(), oclErrorToString(tempVarForDefine)); \
            return TestResult::Error;                                                                                                    \
        }                                                                                                                                \
    }

#define EXPECT_CL_SUCCESS(retVal)                                                                                                        \
    {                                                                                                                                    \
        const auto tempVarForDefine = (retVal);                                                                                          \
        if (tempVarForDefine != CL_SUCCESS) {                                                                                            \
            NON_FATAL_ERROR("EXPECT_CL_SUCCESS", #retVal, std::to_string(tempVarForDefine).c_str(), oclErrorToString(tempVarForDefine)); \
        }                                                                                                                                \
    }

#define CL_SUCCESS_OR_RETURN_VALUE(retVal, value) \
    if ((retVal) != CL_SUCCESS) {                 \
        return (value);                           \
    }

#define CL_SUCCESS_OR_RETURN_FALSE(retVal) \
    CL_SUCCESS_OR_RETURN_VALUE((retVal), false)

#define CL_SUCCESS_OR_RETURN(retVal)                                    \
    {                                                                   \
        const auto tempVarForDefine = (retVal);                         \
        CL_SUCCESS_OR_RETURN_VALUE(tempVarForDefine, tempVarForDefine); \
    }

#define CL_SUCCESS_SUCCESS_OR_RETURN(retVal)                            \
    {                                                                   \
        const auto tempVarForDefine = (retVal);                         \
        CL_SUCCESS_OR_RETURN_VALUE(tempVarForDefine, tempVarForDefine); \
    }

#define CL_SUCCESS_OR_ERROR(retVal, message)                                                                                                \
    {                                                                                                                                       \
        const auto tempVarForDefine = (retVal);                                                                                             \
        if (tempVarForDefine != CL_SUCCESS) {                                                                                               \
            const auto _message = std::string(message) + " (Fatal OpenCL error occurred, retVal=" + std::to_string(tempVarForDefine) + ")"; \
            FATAL_ERROR(_message);                                                                                                          \
        }                                                                                                                                   \
    }
