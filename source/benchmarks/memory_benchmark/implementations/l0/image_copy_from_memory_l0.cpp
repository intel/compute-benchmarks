/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"

#include "definitions/image_copy_from_memory.h"
#include "utility/l0/image_copy_memory_utility_l0.h"

static TestResult run(const ImageCopyFromMemoryArguments &arguments, Statistics &statistics) {
    return L0::measureImageCopyWithMemory(arguments, arguments.sourcePlacement, L0::ImageCopyDirection::FromMemory, statistics);
}

static RegisterTestCaseImplementation<ImageCopyFromMemory> registerTestCase(run, Api::L0);
