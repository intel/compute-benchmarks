/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/submit_graph.h"

#include <gtest/gtest.h>
#include <sycl/sycl.hpp>
#if __has_include(<sycl/ext/oneapi/experimental/graph.hpp>)
#include <sycl/ext/oneapi/experimental/graph.hpp>
#define HAS_SYCL_GRAPH
#endif

static auto enableProfiling = sycl::property::queue::enable_profiling();
static auto inOrder = sycl::property::queue::in_order();

[[maybe_unused]] static const sycl::property_list queueProps[] = {
    sycl::property_list{},
    sycl::property_list{enableProfiling},
    sycl::property_list{inOrder},
    sycl::property_list{inOrder, enableProfiling},
};

static TestResult run([[maybe_unused]] const SubmitGraphArguments &arguments, Statistics &statistics) {
    try {
        MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

        if (isNoopRun()) {
            statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
            return TestResult::Nooped;
        }

#if defined(HAS_SYCL_GRAPH)
        // Setup
        auto queuePropsIndex = 0;
        queuePropsIndex |= arguments.useProfiling ? 0x1 : 0;
        queuePropsIndex |= arguments.inOrderQueue ? 0x2 : 0;
        sycl::queue queue{queueProps[queuePropsIndex]};

        assert(queue.has_property<sycl::property::queue::in_order>() == arguments.inOrderQueue);
        assert(queue.has_property<sycl::property::queue::enable_profiling>() == arguments.useProfiling);

        Timer timer;
        const size_t gws = 1u;
        const size_t lws = 1u;
        sycl::nd_range<1> range(gws, lws);

        // Create kernel
        int kernelOperationsCount = static_cast<int>(arguments.kernelExecutionTime);
        [[maybe_unused]] const auto eat_time = [=]([[maybe_unused]] auto u) {
            if (kernelOperationsCount > 4) {
                volatile int value = kernelOperationsCount;
                while (value) {
                    value = value - 1;
                }
            }
        };

        // Building the graph without explicit dependencies
        sycl::ext::oneapi::experimental::command_graph graph(queue.get_context(), queue.get_device());
        if (arguments.useExplicit) {
            for (auto iteration = 0u; iteration < arguments.numKernels; iteration++) {
                graph.add([&](sycl::handler &h) { h.parallel_for(range, eat_time); });
            }
            if (arguments.useHostTasks) {
                graph.add([&](sycl::handler &h) { h.host_task([=]() { eat_time(0); }); });
            }
        } else {
            graph.begin_recording(queue);
            for (auto iteration = 0u; iteration < arguments.numKernels; iteration++) {
                queue.parallel_for(range, eat_time);
            }
            if (arguments.useHostTasks) {
                queue.submit([&](sycl::handler &cgh) {
                    cgh.host_task([=]() { eat_time(0); });
                });
            }
            graph.end_recording();
        }

        // Finalize the graph
        auto executable_graph = graph.finalize();

        // Warmup
        if (!arguments.useEvents) {
            sycl::ext::oneapi::experimental::execute_graph(queue, executable_graph);
            queue.wait();
        } else {
            sycl::event event = queue.ext_oneapi_graph(executable_graph);
            event.wait();
        }

        // Benchmark
        for (auto i = 0u; i < arguments.iterations; i++) {
            timer.measureStart();
            if (!arguments.useEvents) {
                sycl::ext::oneapi::experimental::execute_graph(queue, executable_graph);
                if (!arguments.measureCompletionTime) {
                    timer.measureEnd();
                }
                queue.wait();

            } else {
                sycl::event event = queue.ext_oneapi_graph(executable_graph);
                if (!arguments.measureCompletionTime) {
                    timer.measureEnd();
                }
                event.wait();
            }

            if (arguments.measureCompletionTime) {
                timer.measureEnd();
            }
            statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
        }

#endif
    } catch (sycl::exception &e) {
        std::cerr << "Caught SYCL exception: " << e.what() << "\n";
        return TestResult::Error;
    }

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<SubmitGraph> registerTestCase(run, Api::SYCL);
