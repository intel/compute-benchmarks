/*
 * Copyright (C) 2023-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <list>
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/submit_graph.h"

#include <sycl/sycl.hpp>
#include <sycl/ext/oneapi/experimental/graph.hpp>

#include <gtest/gtest.h>


using namespace sycl;
namespace sycl_ext = sycl::ext::oneapi::experimental;
 
constexpr std::size_t N = 1024;

static TestResult run(const SubmitGraphArguments & arguments, Statistics & statistics) {
    
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);
    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    std::size_t numK = arguments.numKernels;
    std::cout << "run submit_graph_sycl for " << numK << " kernels\n";

    Timer timer;
    queue Queue{sycl::default_selector_v};
   
   
    sycl_ext::command_graph Graph(Queue.get_context(), Queue.get_device());

    float ** Ptr = sycl::malloc_shared<float*>(numK, Queue);
    for (std::size_t idx = 0; idx < numK; idx++)
    {
        Ptr[idx] = sycl::malloc_device<float>(N, Queue); 
    }
    
    timer.measureStart();

    Graph.begin_recording(Queue);
    
    event InitEvent = Queue.submit([&](handler& CGH) {
    CGH.parallel_for(range<1>(N), [=](item<1> id) {
        for (std::size_t idx = 0; idx < numK; idx++) {
            Ptr[idx][id] = static_cast<float>(idx);
        }
        });
    });
    
    for (std::size_t idx = 0; idx < numK; idx++) {

        event ev = Queue.submit([&](handler& CGH) {
        CGH.depends_on(InitEvent);
        CGH.parallel_for(range<1>(N), [=](item<1> id) {
            Ptr[idx][id] += 1.0f;
        });
        });  
    }

    Graph.end_recording(Queue);

    auto ExecGraph = Graph.finalize();
    Queue.ext_oneapi_graph(ExecGraph);
    Queue.wait_and_throw();

    timer.measureEnd();
    statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());

    for (std::size_t idx = 0; idx < numK; idx++) {
        sycl::free(Ptr[idx], Queue);
    }
    sycl::free(Ptr, Queue);

    return TestResult::Success;
}; 

[[maybe_unused]] static RegisterTestCaseImplementation<SubmitGraph> registerTestCase(run, Api::SYCL);