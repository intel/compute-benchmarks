/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/sycl/sycl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/matrix_multiply.h"

static TestResult run(const MatrixMultiplyArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::GigabytesPerSecond, MeasurementType::Gpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Prepare data
    const size_t sizeInElements = arguments.numberOfElementsX * arguments.numberOfElementsY * arguments.numberOfElementsZ;
    const size_t sizeInBytes = sizeInElements * sizeof(int);

    std::vector<int32_t> dataX(sizeInElements, 0);
    std::vector<int32_t> dataY(sizeInElements, 0);
    std::vector<int32_t> results(sizeInElements, 0);
    std::vector<int32_t> resultsFromRun(sizeInElements, 0);

    const size_t gws[] = {arguments.numberOfElementsX, arguments.numberOfElementsY, arguments.numberOfElementsZ};

    int counter = 0u;

    for (auto z = 0u; z < gws[2]; z++) {
        for (auto y = 0u; y < gws[1]; y++) {
            for (auto x = 0u; x < gws[0]; x++) {
                auto index = x + y * gws[0] + z * gws[0] * gws[1];
                dataX[index] = counter++;
                dataY[index] = counter++;
                results[index] = dataX[index] + dataY[index];
            }
        }
    }

    Sycl sycl{sycl::property::queue::enable_profiling{}};
    const size_t totalElements = gws[0] * gws[1] * gws[2];
    {
        sycl::buffer<int32_t, 1> dataXBuf(dataX.data(), sycl::range<1>{totalElements});
        sycl::buffer<int32_t, 1> dataYBuf(dataY.data(), sycl::range<1>{totalElements});
        sycl::buffer<int32_t, 1> resultsBuf(resultsFromRun.data(), sycl::range<1>{totalElements});

        auto commandList = [&](sycl::handler &cgh) {
            auto dataXDev = dataXBuf.get_access<sycl::access_mode::read>(cgh);
            auto dataYDev = dataYBuf.get_access<sycl::access_mode::read>(cgh);
            auto resultsDev = resultsBuf.get_access<sycl::access_mode::discard_write>(cgh);

            cgh.parallel_for<class MatrixMultiplyKernel>(
                sycl::range<1>{totalElements},
                [=](sycl::item<1> item) {
                    resultsDev[item.get_id()] = dataXDev[item.get_id()] + dataYDev[item.get_id()];
                });
        };

        for (auto i = 0u; i < arguments.iterations; i++) {
            auto profileEvent = sycl.queue.submit(commandList);
            profileEvent.wait();
            auto startTime = profileEvent.get_profiling_info<sycl::info::event_profiling::command_start>();
            auto endTime = profileEvent.get_profiling_info<sycl::info::event_profiling::command_end>();
            auto timeNs = endTime - startTime;

            statistics.pushValue(std::chrono::nanoseconds{timeNs}, sizeInBytes * 3, typeSelector.getUnit(), typeSelector.getType(), "bw");
        }
    }
    if (!std::equal(std::begin(resultsFromRun), std::end(resultsFromRun), std::begin(results), std::end(results))) {
        return TestResult::VerificationFail;
    }

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<MatrixMultiply> registerTestCase(run, Api::SYCL);
