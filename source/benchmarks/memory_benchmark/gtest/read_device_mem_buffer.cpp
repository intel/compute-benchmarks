/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/read_device_mem_buffer.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<ReadDeviceMemBuffer> registerTestCase{};

class ReadDeviceMemBufferTest : public ::testing::TestWithParam<std::tuple<size_t, bool>> {
};

TEST_P(ReadDeviceMemBufferTest, Test) {
    ReadDeviceMemBufferArguments args;
    args.api = Api::OpenCL;
    args.size = std::get<0>(GetParam());
    args.compressed = std::get<1>(GetParam());

    ReadDeviceMemBuffer test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    ReadDeviceMemBufferTest,
    ReadDeviceMemBufferTest,
    ::testing::Combine(
        ::testing::Values(64 * kiloByte, 128 * kiloByte, 512 * kiloByte, 1 * megaByte, 4 * megaByte, 16 * megaByte, 32 * megaByte, 64 * megaByte, 128 * megaByte, 256 * megaByte),
        ::testing::Values(false, true)));
