/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct CreateBufferArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument bufferSize;
    BooleanArgument allocateAll;
    BooleanArgument readOnly;
    BooleanArgument copyHostPtr;
    BooleanArgument forceHostMemoryIntel;

    CreateBufferArguments()
        : bufferSize(*this, "bufferSize", "Buffer size"),
          allocateAll(*this, "allocateAll", "Free buffers at the end of test, as opposed to freeing between iterations. Should disallow resource reuse"),
          readOnly(*this, "readOnly", "Read only buffer"),
          copyHostPtr(*this, "copyHostPtr", "CL_MEM_COPY_HOST_PTR flag"),
          forceHostMemoryIntel(*this, "forceHostMemoryIntel", "CL_MEM_FORCE_HOST_MEMORY_INTEL flag") {}
};

struct CreateBuffer : TestCase<CreateBufferArguments> {
    using TestCase<CreateBufferArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "CreateBuffer";
    }

    std::string getHelp() const override {
        return "measures time spent in clCreateBuffer on CPU.";
    }
};
