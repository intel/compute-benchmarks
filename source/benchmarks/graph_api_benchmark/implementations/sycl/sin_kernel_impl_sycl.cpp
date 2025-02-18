/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "sin_kernel_impl_sycl.h"

#include "framework/sycl/sycl.h"
#include "framework/test_case/test_case.h"

#include <iostream>
#include <math.h>
#include <sycl/sycl.hpp>

SinKernelGraphSYCL::DataFloatPtr SinKernelGraphSYCL::allocDevice(uint32_t count) {
    auto deviceptr = sycl::malloc_device<float>(count, *queue);
    auto copied = queue;
    return SinKernelGraphSYCL::DataFloatPtr(static_cast<float *>(deviceptr), [copied](float *ptr) {
        sycl::free(ptr, *copied);
    });
}

SinKernelGraphSYCL::DataFloatPtr SinKernelGraphSYCL::allocHost(uint32_t count) {
    auto hostptr = sycl::malloc_host<float>(count, *queue);
    auto copied = queue;
    return SinKernelGraphSYCL::DataFloatPtr(static_cast<float *>(hostptr), [copied](float *ptr) {
        sycl::free(ptr, *copied);
    });
}

TestResult SinKernelGraphSYCL::init() {
    queue = std::make_shared<sycl::queue>(sycl::gpu_selector_v, sycl::property_list{sycl::property::queue::in_order()});

    if (!queue->get_device().has(sycl::aspect::ext_oneapi_limited_graph)) {
        return TestResult::DeviceNotCapable;
    }

    if (!queue->get_device().has(sycl::aspect::usm_host_allocations) &&
        !queue->get_device().has(sycl::aspect::usm_device_allocations)) {
        return TestResult::DeviceNotCapable;
    }

    return TestResult::Success;
}

TestResult SinKernelGraphSYCL::runKernels() {
    float *source = graphInputData.get();
    float *dest = graphOutputData.get();

    queue->submit([&](sycl::handler &h) {
        h.parallel_for(size, [=](sycl::item<1> item) {
            int idx = item.get_id(0);
            dest[idx] = source[idx];
        });
    });

    for (uint32_t i = 0; i < numKernels; ++i) {
        std::swap(graphInputData, graphOutputData);

        source = graphInputData.get();
        dest = graphOutputData.get();

        queue->submit(
            [&](sycl::handler &h) {
                h.parallel_for(size, [=](sycl::item<1> item) {
                    int idx = item.get_id(0);
                    dest[idx] = sin(source[idx]);
                });
            });
    }

    if (numKernels % 2 != 0) {
        std::swap(graphInputData, graphOutputData);
    }

    return TestResult::Success;
}

TestResult SinKernelGraphSYCL::recordGraph() {
    graph = sycl_ext::command_graph<sycl_ext::graph_state::modifiable>(*queue);

    graph->begin_recording(*queue);
    runKernels();
    graph->end_recording();

    execGraph = sycl_ext::command_graph<sycl_ext::graph_state::executable>(graph->finalize());

    return TestResult::Success;
}

TestResult SinKernelGraphSYCL::waitCompletion() {
    queue->wait();

    return TestResult::Success;
}

TestResult SinKernelGraphSYCL::readResults(float *output_h) {
    queue->memcpy(output_h, graphOutputData.get(), size * sizeof(float));
    queue->wait();

    return TestResult::Success;
}

TestResult SinKernelGraphSYCL::runGraph(float *input_h) {
    queue->memcpy(graphInputData.get(), input_h, size * sizeof(float));
    queue->ext_oneapi_graph(*execGraph);

    return TestResult::Success;
}

TestResult SinKernelGraphSYCL::runEager(float *input_h) {
    queue->memcpy(graphInputData.get(), input_h, size * sizeof(float));
    runKernels();

    return TestResult::Success;
}

SinKernelGraphSYCL::~SinKernelGraphSYCL() {
}
