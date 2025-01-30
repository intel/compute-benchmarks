/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/read_after_atomic_write.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<ReadAfterAtomicWrite> registerTestCase{};

class ReadAfterAtomicWriteTest : public ::testing::TestWithParam<std::tuple<size_t, bool, bool, bool>> {
};

TEST_P(ReadAfterAtomicWriteTest, Test) {
    ReadAfterAtomicWriteArguments args{};
    args.api = Api::OpenCL;
    args.workgroupSize = std::get<0>(GetParam());
    args.atomic = std::get<1>(GetParam());
    args.shuffleRead = std::get<2>(GetParam());
    args.useEvents = std::get<3>(GetParam());

    ReadAfterAtomicWrite test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    ReadAfterAtomicWriteTest,
    ReadAfterAtomicWriteTest,
    ::testing::Combine(
        ::testing::Values(64, 128, 256, 512),
        ::testing::Values(false, true),
        ::testing::Values(false, true),
        ::testing::Values(true)));

INSTANTIATE_TEST_SUITE_P(
    ReadAfterAtomicWriteTestLIMITED,
    ReadAfterAtomicWriteTest,
    ::testing::Combine(
        ::testing::Values(256),
        ::testing::Values(true),
        ::testing::Values(false, true),
        ::testing::Values(true)));
