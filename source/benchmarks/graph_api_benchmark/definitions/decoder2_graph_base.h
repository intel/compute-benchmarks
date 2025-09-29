/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/benchmark_info.h"
#include "framework/configuration.h"

#include "decoder2_graph.h"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>

template <typename Decoder2GraphImpl>
class Decoder2GraphBase {
  public:
    Decoder2GraphBase(const Decoder2GraphArguments &arguments)
        : size(1u),
          iterations(static_cast<uint32_t>(arguments.iterations)),
          numTokens(static_cast<uint32_t>(arguments.numTokens)),
          useGraphs(arguments.useGraphs),
          emulateGraphs(arguments.emulateGraphs),
          useHostTasks(arguments.useHostTasks) {}

    using DataIntPtr = std::unique_ptr<int, std::function<void(int *)>>;

    DataIntPtr allocDevice(uint32_t count) {
        return static_cast<Decoder2GraphImpl *>(this)->allocDevice(count);
    }

    TestResult clearDeviceBuffer(int *devicePtr, uint32_t count) {
        return static_cast<Decoder2GraphImpl *>(this)->clearDeviceBuffer(devicePtr, count);
    }

    TestResult init() {
        return static_cast<Decoder2GraphImpl *>(this)->init();
    }

    TestResult destroy() {
        return static_cast<Decoder2GraphImpl *>(this)->destroy();
    }

    TestResult waitCompletion() {
        return static_cast<Decoder2GraphImpl *>(this)->waitCompletion();
    }

    TestResult recordGraph() {
        return static_cast<Decoder2GraphImpl *>(this)->recordGraph();
    }

    TestResult runGraph() {
        return static_cast<Decoder2GraphImpl *>(this)->runGraph();
    }

    TestResult readResults(int *actualSum, int *actualSignalCount) {
        return static_cast<Decoder2GraphImpl *>(this)->readResults(actualSum, actualSignalCount);
    }

    bool isUnsupported() {
        return static_cast<Decoder2GraphImpl *>(this)->isUnsupported();
    }

    TestResult calcRefResults(int *refSum, int *refSignalCount) {
        // Our kernel performs INCREMENTS_PER_KERNEL increments. The final result is this times the number of total kernels ran
        *refSum = numTokens * LAYER_NUM * KERNELS_PER_LAYER * INCREMENTS_PER_KERNEL;
        // Since we reset the signal for each layer, we should compare to the number of increments of this counter per layer.
        // This is done once per layer for the CPU and once per layer from the graph host task with host tasks. Without host
        // tasks, the signal is never used.
        *refSignalCount = useHostTasks ? 2 * LAYER_NUM : 0;
        return TestResult::Success;
    }

    bool checkResults(int refSignalCount, int refSum, int actualSignalCount, int actualSum) {
        bool isCorrect = true;
        if (refSum != actualSum) {
            std::cout << "Expected kernel counter: " << refSum << ", Actual kernel counter: " << actualSum << std::endl;
            isCorrect = false;
        }
        if (refSignalCount != actualSignalCount) {
            std::cout << "Expected signal count: " << refSignalCount << ", Actual signal count: " << actualSignalCount << std::endl;
            isCorrect = false;
        }
        return isCorrect;
    }

    TestResult runIteration() {
        for (uint32_t tokenIdx = 0; tokenIdx < numTokens; ++tokenIdx) {
            // Perform CPU work in coordination with GPU. Note that the lock based GPU - CPU coordination via 'com' is
            // only used when host tasks are enabled and extra queue waits are used without host tasks. A single graph
            // is executed per token if host tasks are enabled. Otherwise, we execute a single graph per layer.
            if (useHostTasks) {
                com.reset();
                ASSERT_TEST_RESULT_SUCCESS(runGraph());
            }
            for (uint32_t i = 0; i < LAYER_NUM; ++i) {
                if (useHostTasks) {
                    com.waitGPU();
                } else {
                    ASSERT_TEST_RESULT_SUCCESS(runGraph());
                    // Emulation modes must explicitly synchronize between graph submissions
                    if (emulateGraphs) {
                        waitCompletion();
                    }
                }
                if (useHostTasks)
                    com.notify();
            }
            // We only need to wait on the queue once between tokens outside the loop with the host task case and in non
            // emulation modes.
            if (useHostTasks || !emulateGraphs) {
                waitCompletion();
            }
        }
        return TestResult::Success;
    }

    TestResult run(Statistics &statistics) {
        MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

        if (isUnsupported()) {
            return TestResult::ApiNotCapable;
        } else if (isNoopRun()) {
            statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
            return TestResult::Nooped;
        }

        ASSERT_TEST_RESULT_SUCCESS(init());

        TestResult result = TestResult::Success;
        Timer timer;

        graphData = allocDevice(size);
        // Warmup & correctness check
        {
            ASSERT_TEST_RESULT_SUCCESS(recordGraph());
            ASSERT_TEST_RESULT_SUCCESS(runIteration());
            int refSum = 0, refSignalCount = 0, actualSum = 0, actualSignalCount = 0;
            ASSERT_TEST_RESULT_SUCCESS(calcRefResults(&refSum, &refSignalCount));
            ASSERT_TEST_RESULT_SUCCESS(readResults(&actualSum, &actualSignalCount));
            if (!checkResults(refSignalCount, refSum, actualSignalCount, actualSum)) {
                std::cout << "Check FAILED" << std::endl;
                return TestResult::Error;
            }
        }

        // Timed runs
        for (uint32_t i = 0; i < iterations; ++i) {
            clearDeviceBuffer(graphData.get(), size);
            timer.measureStart();
            ASSERT_TEST_RESULT_SUCCESS(runIteration());
            timer.measureEnd();
            // Normalize the results to report average time per token. We expect a constant time when scaling the number
            // of tokens.
            statistics.pushValue(timer.get() / numTokens, typeSelector.getUnit(), typeSelector.getType());
        }
        ASSERT_TEST_RESULT_SUCCESS(destroy());
        return result;
    }

    void gpuHostTask() {
        com.notify();
        com.waitCPU();
    }

  protected:
    ~Decoder2GraphBase() = default;
    // TODO: In the future we may wish to parameterize these for simulating different LLM workloads
    static constexpr uint32_t LAYER_NUM = 95;
    static constexpr uint32_t KERNELS_PER_LAYER = 59;
    // The original decoder2 benchmark uses a timeout of 700 us. This change to 1 us saves benchmark runtime with the
    // side effect of graph execution taking up a larger percentage of the total runtime compared to CPU work.
    static constexpr uint32_t CPU_TIMEOUT = 1;
    static constexpr uint32_t INCREMENTS_PER_KERNEL = 1;

    struct communicate {
        std::mutex m;
        std::condition_variable cv;
        std::unique_ptr<int> canBegin;

        communicate()
            : canBegin(std::make_unique<int>(0)) {
            reset();
        }
        void reset() {
            *canBegin = 0;
        }
        // Notify CPU|GPU task done
        void notify() {
            {
                std::unique_lock lk(m);
                (*canBegin)++;
            }
            cv.notify_one();
        };
        // wait for the CPU task to be done
        void waitCPU() {
            std::unique_lock lk(m);
            cv.wait(lk, [this] { return *canBegin % 2 == 0; });
        };
        // wait for the GPU task to be done
        void waitGPU() {
            std::unique_lock lk(m);
            cv.wait(lk, [this] { return *canBegin % 2 == 1; });
        };
    } com;

    uint32_t size;
    uint32_t iterations;
    uint32_t numTokens;
    bool useGraphs;
    bool emulateGraphs;
    bool useHostTasks;

    DataIntPtr graphData;
};
