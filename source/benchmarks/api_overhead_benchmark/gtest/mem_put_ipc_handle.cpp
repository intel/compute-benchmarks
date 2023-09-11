/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/mem_put_ipc_handle.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<MemPutIpcHandle> registerTestCase{};

class MemPutIpcHandleTest : public ::testing::TestWithParam<std::tuple<UsmMemoryPlacement, size_t>> {
};

TEST_P(MemPutIpcHandleTest, Test) {
    MemPutIpcHandleArguments args{};
    args.api = Api::L0;
    args.sourcePlacement = std::get<0>(GetParam());
    args.AllocationsCount = std::get<1>(GetParam());
    MemPutIpcHandle test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    MemPutIpcHandleTest,
    MemPutIpcHandleTest,
    ::testing::Combine(
        ::testing::Values(UsmMemoryPlacement::Host, UsmMemoryPlacement::Device),
        ::testing::Values(1, 10u, 100u)));
