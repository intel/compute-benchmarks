/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"

#include "definitions/image_copy_to_memory.h"
#include "utility/l0/image_copy_memory_utility_l0.h"

static TestResult run(const ImageCopyToMemoryArguments &arguments, Statistics &statistics) {
    return L0::measureImageCopyWithMemory(arguments, arguments.destinationPlacement, L0::ImageCopyDirection::ToMemory, statistics);
}

static RegisterTestCaseImplementation<ImageCopyToMemory> registerTestCase(run, Api::L0);
