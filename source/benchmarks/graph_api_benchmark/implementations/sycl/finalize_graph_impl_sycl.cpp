/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/argument/enum/graph_structure_argument.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/finalize_graph.h"

#include <gtest/gtest.h>
#include <sycl/sycl.hpp>
#if __has_include(<sycl/ext/oneapi/experimental/graph.hpp>)
#include "graph_helpers.h"

#include <sycl/ext/oneapi/experimental/graph.hpp>
#define HAS_SYCL_GRAPH

#endif

static TestResult run([[maybe_unused]] const FinalizeGraphArguments &arguments, Statistics &statistics) {
    try {
        MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

        if (isNoopRun()) {
            statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
            return TestResult::Nooped;
        }

#if defined(HAS_SYCL_GRAPH)
        namespace sycl_ext = sycl::ext::oneapi::experimental;

        // Setup
        sycl::queue queue{};

        Timer timer;

        auto graphStructure = arguments.graphStructure;
        // Building the graph
        sycl_ext::command_graph graph = graph_helpers::constructGraph(graphStructure, queue.get_context(), queue.get_device());

        // Do warmup to eliminate kernel compilation or other noise from graph finalize
        graph.finalize();

        // Benchmarking finalize time
        for (size_t iteration = 0; iteration < arguments.iterations; iteration++) {
            auto LocalGraph = arguments.rebuildGraphEveryIter ? graph_helpers::constructGraph(graphStructure, queue.get_context(), queue.get_device()) : graph;
            timer.measureStart();
            // Finalize the graph
            auto executable_graph = graph.finalize();
            timer.measureEnd();
            statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
        }
#endif
    } catch (sycl::exception &e) {
        std::cerr << "Caught SYCL exception: " << e.what() << "\n";
        return TestResult::Error;
    }

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<FinalizeGraph> registerTestCase(run, Api::SYCL);
