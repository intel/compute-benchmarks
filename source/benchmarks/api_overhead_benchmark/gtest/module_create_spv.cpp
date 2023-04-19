/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/module_create_spv.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<ModuleCreateSpv> registerTestCase{};

class ModuleCreateSpvTest : public ::testing::TestWithParam<std::tuple<std::string>> {
};

TEST_P(ModuleCreateSpvTest, Test) {
    ModuleCreateSpvArguments args{};
    args.api = Api::L0;
    args.kernelName = std::get<0>(GetParam());

    ModuleCreateSpv test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    ModuleCreateSpvTest,
    ModuleCreateSpvTest,
    ::testing::Combine(
        ::testing::Values("api_overhead_benchmark_write_sum_local.spv",
                          "api_overhead_benchmark_global_consts.spv")));
