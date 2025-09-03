/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/write_buffer_rect.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<WriteBufferRect> registerTestCase{};

using Tuple = ThreeComponentUintArgument::TupleType;
class WriteBufferRectTestEntireBuffer : public ::testing::TestWithParam<std::tuple<bool, Tuple, bool, HostptrReuseMode, BufferContents>> {
};

TEST_P(WriteBufferRectTestEntireBuffer, Test) {
    const auto region = std::get<1>(GetParam());
    const auto size = std::get<0>(region) * std::get<1>(region) * std::get<2>(region);

    WriteBufferRectArguments args;
    args.api = Api::OpenCL;
    args.size = size;
    args.compressed = std::get<0>(GetParam());
    args.origin = Tuple(0, 0, 0);
    args.region = std::get<1>(GetParam());
    args.rPitch = args.region[0];
    args.sPitch = args.region[0] * args.region[1];
    args.inOrderQueue = std::get<2>(GetParam());
    args.reuse = std::get<3>(GetParam());
    args.contents = std::get<4>(GetParam());

    WriteBufferRect test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    WriteBufferRectTestEntireBuffer,
    WriteBufferRectTestEntireBuffer,
    ::testing::Combine(
        ::testing::Values(false, true),
        ::testing::Values(Tuple(128, 128, 1), Tuple(128, 128, 128), Tuple(128, 1024, 1024), Tuple(1024, 16, 1024), Tuple(1024, 1024, 16)),
        ::testing::Values(false, true),
        ::testing::Values(HostptrReuseMode::Aligned4KB, HostptrReuseMode::Misaligned),
        ::testing::Values(BufferContents::Zeros)));
