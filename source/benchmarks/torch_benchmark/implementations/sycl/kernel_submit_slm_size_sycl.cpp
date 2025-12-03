/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/kernel_submit_slm_size.h"

#include <sycl/sycl.hpp>

using data_type = float;

namespace syclex = sycl::ext::oneapi::experimental;

static void submit_kernel_slm(sycl::queue &q, data_type *out, const std::size_t slm_num) {

    const size_t wgs = std::min(slm_num, 1024ul);
    const size_t wgc = 1;

    syclex::nd_launch(q,
                      syclex::launch_config(sycl::nd_range<1>{wgc * wgs, wgs},
                                            syclex::properties{syclex::work_group_scratch_size(slm_num * sizeof(data_type))}),
                      [=](sycl::nd_item<1> item) {
                          data_type *slm = (data_type *)syclex::get_work_group_scratch_memory();
                          const size_t local_id = item.get_local_id(0);
                          const size_t local_size = item.get_local_range(0);
                          for (size_t i = local_id; i < slm_num; i += local_size) {
                              slm[i] = 0.1f * i;
                          }
                          item.barrier(sycl::access::fence_space::local_space);
                          if (local_id == 0) {
                              out[0] = slm[slm_num - 1];
                              out[1] = 13.0f;
                          }
                      });
}

static TestResult run(const KernelSubmitSlmSizeArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // sycl::device dev = sycl::device([](const sycl::device&){return 0;}); // use it to run on CPU
    sycl::device dev = sycl::device(sycl::gpu_selector_v);
    sycl::queue q(dev, sycl::property::queue::in_order());

    const size_t max_slm_size = dev.get_info<sycl::info::device::local_mem_size>();

    int slm_num = arguments.slmNum;
    if (slm_num == -1) {
        slm_num = max_slm_size / sizeof(data_type);
    }

    const size_t slm_size = slm_num * sizeof(data_type);
    if (slm_size > max_slm_size) {
        return TestResult::Error;
    }

    data_type *d_out = sycl::malloc_device<data_type>(2, q);
    if (d_out == nullptr) {
        return TestResult::Error;
    }

    // Warmup
    for (int i = 0; i < arguments.warmupIterations; i++) {
        submit_kernel_slm(q, d_out, slm_num);
    }
    q.wait();

    for (size_t i = 0; i < arguments.iterations; ++i) {

        Timer timer;
        timer.measureStart();
        submit_kernel_slm(q, d_out, slm_num);
        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());

        // expect a wait here after a batch of submissions
        if (i > 0 && (i % arguments.batchSize) == 0) {
            q.wait();
        }
    }

    q.wait();

    // clean up
    sycl::free(d_out, q);

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<KernelSubmitSlmSize> registerTestCase(run, Api::SYCL);