/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "program_helper_ocl.h"

#include "framework/ocl/utility/error.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/string_utils.h"

TestResult OCL::ProgramHelperOcl::buildProgramFromSource(cl_context context, cl_device_id device, const char *source, size_t sourceLength, const char *compileOptions, cl_program &outProgram) {
    FATAL_ERROR_IF(outProgram != nullptr, "Non-null program passed");

    cl_int retVal{};
    cl_program program = clCreateProgramWithSource(context, 1, &source, &sourceLength, &retVal);
    ASSERT_CL_SUCCESS(retVal);

    retVal = clBuildProgram(program, 1, &device, compileOptions, nullptr, nullptr);
    if (retVal != CL_SUCCESS) {
        size_t logSize{};
        ASSERT_CL_SUCCESS(clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &logSize));
        auto log = std::make_unique<char[]>(logSize);
        ASSERT_CL_SUCCESS(clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, logSize, log.get(), nullptr));
        const std::string indentedString = indentString(log.get(), 16);
        std::cout << indentedString << '\n';
        return TestResult::KernelBuildError;
    }

    outProgram = program;
    return TestResult::Success;
}

TestResult OCL::ProgramHelperOcl::buildProgramFromSourceFile(cl_context context, cl_device_id device, const char *sourceFileName, const char *compileOptions, cl_program &outProgram) {
    const std::vector<uint8_t> kernelSource = FileHelper::loadTextFile(sourceFileName);
    if (kernelSource.size() == 0) {
        return TestResult::KernelNotFound;
    }
    const char *source = reinterpret_cast<const char *>(kernelSource.data());
    const size_t sourceLength = kernelSource.size();
    return buildProgramFromSource(context, device, source, sourceLength, compileOptions, outProgram);
}
