/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"
#include "framework/utility/combo_profiler.h"

#include "definitions/kernel_submit_slm_size.h"

#include <cmath>
#include <sycl/sycl.hpp>

using data_type = float;
const int WARMUP_ITERATIONS = 3;

namespace syclex = sycl::ext::oneapi::experimental;

template <typename data_type> // in case somebody changes data_type to integer type later
static bool data_type_equal(data_type a, data_type b) {
    return a == b;
}

template <>
bool data_type_equal<float>(float a, float b) {
    const float epsilon = 1e-3f;
    return std::fabs(a - b) <= epsilon * std::max(std::fabs(a), std::fabs(b));
}

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
                              out[1] = 13.0f;
                          }
                          if ((slm_num - 1 - local_id) % local_size == 0) {
                              out[0] = slm[slm_num - 1];
                          }
                      });
}

static auto enableProfiling = sycl::property::queue::enable_profiling();
static auto inOrder = sycl::property::queue::in_order();

static TestResult run(const KernelSubmitSlmSizeArguments &args, Statistics &statistics) {
    ComboProfilerWithStats profiler(Configuration::get().profilerType);

    if (isNoopRun()) {
        profiler.pushNoop(statistics);
        return TestResult::Nooped;
    }

    // sycl::device dev = sycl::device([](const sycl::device&){return 0;}); // use it to run on CPU
    sycl::device dev = sycl::device(sycl::gpu_selector_v);
    sycl::queue q(dev, args.useProfiling ? sycl::property_list{inOrder, enableProfiling} : sycl::property_list{inOrder});

    const size_t max_slm_size = dev.get_info<sycl::info::device::local_mem_size>();

    int slm_num = args.slmNum;
    if (slm_num == -1) {
        slm_num = max_slm_size / sizeof(data_type);
    }

    const size_t slm_size = slm_num * sizeof(data_type);
    if (slm_size > max_slm_size) {
        return TestResult::InvalidArgs;
    }

    data_type *d_out = sycl::malloc_device<data_type>(2, q);
    if (d_out == nullptr) {
        return TestResult::Error;
    }

    // Warmup
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        submit_kernel_slm(q, d_out, slm_num);
    }
    q.wait();

    for (size_t i = 0; i < args.iterations; ++i) {

        profiler.measureStart();

        for (size_t j = 0; j < static_cast<size_t>(args.kernelBatchSize); ++j) {
            submit_kernel_slm(q, d_out, slm_num);
        }

        if (!args.measureCompletion) {
            profiler.measureEnd();
            profiler.pushStats(statistics);
        }

        q.wait();

        if (args.measureCompletion) {
            profiler.measureEnd();
            profiler.pushStats(statistics);
        }
    }

    // verification
    data_type out[2] = {0.0f, 0.0f};

    q.memcpy(out, d_out, 2 * sizeof(data_type)).wait();
    const data_type expected = 0.1f * (slm_num - 1);
    if (!data_type_equal(out[0], expected) || !data_type_equal(out[1], 13.0f)) {
        std::cerr << "Verification failed: expected: " << expected << " and 13.0, but got " << out[0] << ", " << out[1] << std::endl;
        sycl::free(d_out, q);
        return TestResult::VerificationFail;
    }

    // clean up
    sycl::free(d_out, q);

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<KernelSubmitSlmSize> registerTestCase(run, Api::SYCL);
