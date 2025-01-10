/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/sin_kernel_graph.h"
#include "sin_common.h"

#include <gtest/gtest.h>
#include <math.h>
#include <sycl/sycl.hpp>

namespace sycl_ext = sycl::ext::oneapi::experimental;

static TestResult run(const SinKernelGraphArguments &arguments, Statistics &statistics) {

    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);
    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    std::size_t numKernels = arguments.numKernels;
    bool withGraphs = arguments.withGraphs;

    Timer timer;

    sycl::property_list prop_list{sycl::property::queue::in_order()};

    sycl::queue Queue{sycl::gpu_selector_v, prop_list};

    if (!Queue.get_device().has(sycl::aspect::ext_oneapi_limited_graph)) {
        return TestResult::DeviceNotCapable;
    }

    if (!Queue.get_device().has(sycl::aspect::usm_host_allocations) &&
        !Queue.get_device().has(sycl::aspect::usm_device_allocations)) {
        return TestResult::DeviceNotCapable;
    }

    deviceMemMgr.init(&Queue);

    // shape of model input (ABCD in general)
    constexpr size_t a = 4, b = 16, c = 64, d = 1024;

    // prepare host data for model input
    float *input_h = sycl::malloc_host<float>(a * b * c * d, Queue);

    for (size_t i = 0; i < a * b * c * d; ++i) {
        input_h[i] = random_float();
    }

    // warm up
    float *golden_h = sycl::malloc_host<float>(a * b * c * d * sizeof(float), Queue);

    // Host2Device for model input
    Tensor4D input(a, b, c, d);
    Queue.memcpy(input.data, input_h, input.count() * sizeof(float));

    Tensor4D output = run_model(input, Queue, numKernels);

    Queue.memcpy(golden_h, output.data, output.count() * sizeof(float)).wait();
    deviceMemMgr.free(input.data);
    deviceMemMgr.free(output.data);

    sycl_ext::command_graph Graph{Queue.get_context(), Queue.get_device()};

    Tensor4D gr_input(a, b, c, d);

    Graph.begin_recording({Queue});
    Tensor4D gr_output = run_model(gr_input, Queue, numKernels);
    deviceMemMgr.free(gr_output.data);
    Graph.end_recording();

    auto execGraph = Graph.finalize();

    float *output_h = sycl::malloc_host<float>(gr_output.count() * sizeof(float), Queue);

    sycl::property_list exec_q_prop_list = {sycl::property::queue::in_order()};
    sycl::queue exec_q{sycl::gpu_selector_v, exec_q_prop_list};

    auto check_result = [&] {
        bool ret = true;
        for (int ai = 0; ai < gr_output.A; ++ai) {
            for (int bi = 0; bi < gr_output.B; ++bi) {
                for (int ci = 0; ci < gr_output.C; ++ci) {
                    for (int di = 0; di < gr_output.D; ++di) {
                        int index = di + ci * gr_output.D + bi * gr_output.C * gr_output.D + ai * gr_output.B * gr_output.C * gr_output.D;
                        if (fabs(output_h[index] - golden_h[index]) > 0.0001f) {
                            ret = false;
                            std::cout << "at (" << ai << ", " << bi << ", " << ci << ", " << di << "), expect " << golden_h[index] << ", but got " << output_h[index] << std::endl;
                        }
                    }
                }
            }
        }
        return ret;
    };

    // do the check
    exec_q.memset(gr_output.data, 0, gr_output.count() * sizeof(float)).wait();
    exec_q.memcpy(gr_input.data, input_h, gr_input.count() * sizeof(float));
    exec_q.ext_oneapi_graph(execGraph);

    exec_q.memcpy(output_h, gr_output.data, gr_output.count() * sizeof(float)).wait();

    TestResult result = TestResult::Success;

    exec_q.wait(); // just for sure no pending GPU workloads

    if (!check_result()) {
        std::cout << "Check FAILED" << std::endl;
        result = TestResult::Error;
    } else {
        int repeat = 100;
        for (std::size_t i = 0; i < arguments.iterations; ++i) {
            if (!withGraphs) {
                timer.measureStart();

                for (int i = 0; i < repeat; ++i) {
                    exec_q.memcpy(gr_input.data, input_h, gr_input.count() * sizeof(float));
                    gr_output = run_model(gr_input, exec_q, numKernels);
                    deviceMemMgr.free(gr_output.data);
                }
                exec_q.wait();
                timer.measureEnd();
            } else {
                // run with graph
                timer.measureStart();
                for (int i = 0; i < repeat; ++i) {
                    exec_q.memcpy(gr_input.data, input_h, gr_input.count() * sizeof(float));
                    exec_q.ext_oneapi_graph(execGraph);
                }
                exec_q.wait();
                timer.measureEnd();
            }
            statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
        }
    }
    deviceMemMgr.free(gr_input.data);
    sycl::free(input_h, Queue);
    sycl::free(golden_h, Queue);
    sycl::free(output_h, Queue);

    // make sure all the GPU tasks are done when cleanup
    Queue.wait();

    deviceMemMgr.deinit();
    return result;
}

[[maybe_unused]] static RegisterTestCaseImplementation<SinKernelGraph> registerTestCase(run, Api::SYCL);