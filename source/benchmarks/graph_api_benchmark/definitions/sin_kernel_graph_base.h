/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/benchmark_info.h"

#include "sin_kernel_graph.h"

#include <cmath>
#include <functional>
#include <iostream>
#include <memory>
#include <random>
#include <vector>

class SinKernelGraphBase {
  public:
    SinKernelGraphBase(const SinKernelGraphArguments &arguments)
        : numKernels(arguments.numKernels), size(65536), withGraphs(arguments.withGraphs), withCopyOffload(arguments.withCopyOffload), immediateAppendCmdList(arguments.immediateAppendCmdList), iterations(arguments.iterations), engine(0), distribution(-10.0, 10.0) {};

    using DataFloatPtr = std::unique_ptr<float, std::function<void(float *)>>;

    virtual DataFloatPtr allocDevice(uint32_t count) = 0;
    virtual DataFloatPtr allocHost(uint32_t count) = 0;

    virtual TestResult init() = 0;

    virtual TestResult recordGraph() = 0;
    virtual TestResult readResults(float *output_h) = 0;

    virtual TestResult runGraph(float *input_h) = 0;
    virtual TestResult runEager(float *input_h) = 0;
    virtual TestResult waitCompletion() = 0;

    TestResult calcRefResults(float *input_h, float *golden_h) {
        std::vector<float> buffer0(size);
        std::vector<float> buffer1(size);

        // assign action
        for (uint32_t i = 0; i < size; ++i)
            buffer0[i] = input_h[i];

        // repeat sin action
        for (size_t k = 0; k < numKernels; ++k) {
            std::swap(buffer0, buffer1);
            for (uint32_t i = 0; i < size; ++i)
                buffer0[i] = sin(buffer1[i]);
        }

        if (numKernels % 2 != 0)
            std::swap(buffer0, buffer1);

        for (uint32_t i = 0; i < size; ++i)
            golden_h[i] = buffer0[i];

        return TestResult::Success;
    }

    bool checkResults(float *output_h, float *golden_h) {
        bool ret = true;
        for (uint32_t idx = 0; idx < size; ++idx) {
            if ((fabs(output_h[idx] - golden_h[idx]) > 0.00001f) ||
                (output_h[idx]) == 0.0f) {
                ret = false;
                std::cout << "at (" << idx << "), expected " << golden_h[idx]
                          << ", but got " << output_h[idx] << std::endl;
                return ret;
            }
        }
        return ret;
    }

    float randFloat() {
        return distribution(engine);
    }

    TestResult run(
        Statistics &statistics) {
        MeasurementFields typeSelector(MeasurementUnit::Microseconds,
                                       MeasurementType::Cpu);
        if (isNoopRun()) {
            statistics.pushUnitAndType(typeSelector.getUnit(),
                                       typeSelector.getType());
            return TestResult::Nooped;
        }

        init();

        TestResult result = TestResult::Success;

        Timer timer;

        DataFloatPtr inputData = allocHost(size);
        DataFloatPtr refResult = allocHost(size);
        DataFloatPtr outputData = allocHost(size);

        graphInputData = allocDevice(size);
        graphOutputData = allocDevice(size);

        for (uint32_t i = 0; i < size; ++i) {
            inputData.get()[i] = randFloat();
        }

        // reference results
        ASSERT_TEST_RESULT_SUCCESS(calcRefResults(inputData.get(), refResult.get()));

        if (withGraphs)
            ASSERT_TEST_RESULT_SUCCESS(recordGraph());

        // warm-up & results verification
        {
            if (withGraphs) {
                ASSERT_TEST_RESULT_SUCCESS(runGraph(inputData.get()));
            } else {
                ASSERT_TEST_RESULT_SUCCESS(runEager(inputData.get()));
            }
            ASSERT_TEST_RESULT_SUCCESS(waitCompletion());
            ASSERT_TEST_RESULT_SUCCESS(readResults(outputData.get()));

            // if results don't match, fail the benchmark
            if (!checkResults(outputData.get(), refResult.get())) {
                std::cout << "Check FAILED" << std::endl;
                return TestResult::Error;
            }
        }

        for (uint32_t i = 0; i < iterations; ++i) {
            timer.measureStart();

            if (withGraphs) {
                ASSERT_TEST_RESULT_SUCCESS(runGraph(inputData.get()));
            } else {
                ASSERT_TEST_RESULT_SUCCESS(runEager(inputData.get()));
            }

            ASSERT_TEST_RESULT_SUCCESS(waitCompletion());

            timer.measureEnd();
            statistics.pushValue(timer.get(), typeSelector.getUnit(),
                                 typeSelector.getType());
        }

        return result;
    }

    ~SinKernelGraphBase() = default;

  protected:
    size_t numKernels;
    uint32_t size;
    bool withGraphs;
    bool withCopyOffload;
    bool immediateAppendCmdList;

    size_t iterations;

    float pattern = 123.4567f;

    // device memory
    DataFloatPtr graphInputData;
    DataFloatPtr graphOutputData;

    std::mt19937 engine;
    std::uniform_real_distribution<float> distribution;
};
