/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/data_type_argument.h"
#include "framework/argument/enum/normal_math_operation_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct ReadAfterAtomicWriteArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument workgroupSize;
    BooleanArgument atomic;
    BooleanArgument shuffleRead;

    ReadAfterAtomicWriteArguments()
        : workgroupSize(*this, "wgs", "Workgroup size"),
          atomic(*this, "atomic", "If true, write to global memory will be atomic."),
          shuffleRead(*this, "shuffleRead", "If true, each thread will write and read different memory cell. Otherwise it will be the same one.") {}
};

struct ReadAfterAtomicWrite : TestCase<ReadAfterAtomicWriteArguments> {
    using TestCase<ReadAfterAtomicWriteArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "ReadAfterAtomicWrite";
    }

    std::string getHelp() const override {
        return "enqueues kernel, which writes to global memory using atomic and then reads non atomically";
    }
};
