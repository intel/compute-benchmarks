/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/virtual_mem_query_page_size.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<VirtualMemQueryPageSize> registerTestCase{};

class VirtualMemQueryPageSizeTest : public ::testing::TestWithParam<size_t> {
};

TEST_P(VirtualMemQueryPageSizeTest, Test) {
    VirtualMemQueryPageSizeArguments args{};
    args.api = Api::L0;
    VirtualMemQueryPageSize test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    VirtualMemQueryPageSizeTest,
    VirtualMemQueryPageSizeTest,
    ::testing::Values(Api::L0));
