/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/mem_open_ipc_handle.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<MemOpenIpcHandle> registerTestCase{};

class MemOpenIpcHandleTest : public ::testing::TestWithParam<std::tuple<UsmMemoryPlacement, size_t>> {
};

TEST_P(MemOpenIpcHandleTest, Test) {
    MemOpenIpcHandleArguments args{};
    args.api = Api::L0;
    args.sourcePlacement = std::get<0>(GetParam());
    args.AllocationsCount = std::get<1>(GetParam());
    MemOpenIpcHandle test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    MemOpenIpcHandleTest,
    MemOpenIpcHandleTest,
    ::testing::Combine(
        ::testing::Values(UsmMemoryPlacement::Host, UsmMemoryPlacement::Device),
        ::testing::Values(1, 10u, 100u)));
