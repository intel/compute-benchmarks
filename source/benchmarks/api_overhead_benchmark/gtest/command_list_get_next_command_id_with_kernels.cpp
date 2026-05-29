/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/command_list_get_next_command_id_with_kernels.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<CommandListGetNextCommandIdWithKernels> registerTestCase{};
class CommandListGetNextCommandIdWithKernelsTest : public ::testing::TestWithParam<std::tuple<MutableCommandFlag, size_t>> {
};

TEST_P(CommandListGetNextCommandIdWithKernelsTest, Test) {
    CommandListGetNextCommandIdWithKernelsArguments args{};
    args.api = Api::L0;
    args.mutableCommandFlag = std::get<0>(GetParam());
    args.numKernels = std::get<1>(GetParam());

    CommandListGetNextCommandIdWithKernels test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    CommandListGetNextCommandIdWithKernelsTest,
    CommandListGetNextCommandIdWithKernelsTest,
    ::testing::Combine(
        ::testing::Values(MutableCommandFlag::All, // mutableCommandFlag
                          MutableCommandFlag::KernelArguments,
                          MutableCommandFlag::GroupCount,
                          MutableCommandFlag::GroupSize,
                          MutableCommandFlag::GlobalOffset,
                          MutableCommandFlag::SignalEvent,
                          MutableCommandFlag::WaitEvents,
                          MutableCommandFlag::KernelInstruction,
                          MutableCommandFlag::GraphArguments),
        ::testing::Values(10u))); // numKernels
