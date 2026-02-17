/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/kernel_submit_memory_reuse.h"

#include "framework/enum/data_type.h"
#include "framework/test_case/register_test_case.h"

#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<KernelSubmitMemoryReuse> registerTestCase{};

class KernelSubmitMemoryReuseTest : public ::testing::TestWithParam<std::tuple<Api, uint32_t, DataType, bool>> {
};

TEST_P(KernelSubmitMemoryReuseTest, Test) {
    KernelSubmitMemoryReuseArguments args{};
    args.api = std::get<0>(GetParam());
    args.kernelBatchSize = std::get<1>(GetParam());
    args.kernelDataType = std::get<2>(GetParam());
    args.useEvents = std::get<3>(GetParam());
    KernelSubmitMemoryReuse test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    KernelSubmitMemoryReuseTest,
    KernelSubmitMemoryReuseTest,
    ::testing::Combine(
        ::testing::Values(Api::L0, Api::SYCL, Api::SYCLPREVIEW),
        ::testing::Values(10),                               // kernelBatchSize
        ::testing::Values(DataType::Int32, DataType::Float), // kernelDataType
        ::testing::Values(false, true)));                    // useEvents
