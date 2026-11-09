/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/usm_copy_reserved_memory.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<UsmCopyReservedMemory> registerTestCase{};

class UsmCopyReservedMemoryTest : public ::testing::TestWithParam<std::tuple<size_t, size_t, size_t, bool>> {
};

TEST_P(UsmCopyReservedMemoryTest, Test) {
    UsmCopyReservedMemoryArguments args{};
    args.api = Api::L0;
    args.size = std::get<0>(GetParam());
    args.mapCount = std::get<1>(GetParam());
    args.chunkSize = std::get<2>(GetParam());
    args.measureCompletionTime = std::get<3>(GetParam());

    UsmCopyReservedMemory test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    UsmCopyReservedMemoryTest,
    UsmCopyReservedMemoryTest,
    ::testing::Combine(
        ::testing::Values(64 * kiloByte),
        ::testing::Values(2, 1024, 4096),
        ::testing::Values(64 * kiloByte),
        ::testing::Values(false, true)));
