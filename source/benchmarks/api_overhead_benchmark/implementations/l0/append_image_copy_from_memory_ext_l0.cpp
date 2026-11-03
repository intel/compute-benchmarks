/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"

#include "definitions/append_image_copy_from_memory_ext.h"
#include "utility/l0/append_image_copy_memory_ext_utility_l0.h"

#include <gtest/gtest.h>

static TestResult run(const AppendImageCopyFromMemoryExtArguments &arguments, Statistics &statistics) {
    return measureAppendImageCopyMemoryExt(arguments, arguments.src, statistics,
                                           [](ze_command_list_handle_t cmdList, ze_image_handle_t image, void *buffer, const ze_image_region_t *imageRegion,
                                              uint32_t bufferRowPitch, uint32_t bufferSlicePitch, ze_event_handle_t event) {
                                               return zeCommandListAppendImageCopyFromMemoryExt(cmdList, image, buffer, imageRegion, bufferRowPitch, bufferSlicePitch, event, 0, nullptr);
                                           });
}

static RegisterTestCaseImplementation<AppendImageCopyFromMemoryExt> registerTestCase(run, Api::L0);
