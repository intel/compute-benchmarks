/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/sycl/sycl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/combo_profiler.h"

#include "definitions/kernel_submit_linear_kernel_size.h"
#include "definitions/linear_kernel_size_kernels.h"

#include <sycl/sycl.hpp>

using data_type = double;

namespace syclex = sycl::ext::oneapi::experimental;

static TestResult submit_kernel_linear_kernel_size(sycl::queue &q, const int size, data_type *out) {
    if (size == 32) {
        syclex::single_task(q, [=]() {
            linear_kernel_size_32<data_type>(out);
        });
    } else if (size == 128) {
        syclex::single_task(q, [=]() {
            linear_kernel_size_128<data_type>(out);
        });
    } else if (size == 512) {
        syclex::single_task(q, [=]() {
            linear_kernel_size_512<data_type>(out);
        });
    } else if (size == 1024) {
        syclex::single_task(q, [=]() {
            linear_kernel_size_1024<data_type>(out);
        });
    } else if (size == 5120) {
        syclex::single_task(q, [=]() {
            linear_kernel_size_5120<data_type>(out);
        });
    } else {
        std::cerr << "Invalid kernel size: " << size << ". Allowed sizes are 32, 128, 512, 1024, 5120." << std::endl;
        return TestResult::Error;
    }

    return TestResult::Success;
}

static TestResult run(const KernelSubmitLinearKernelSizeArguments &args, Statistics &statistics) {
    ComboProfilerWithStats profiler(Configuration::get().profilerType);

    if (isNoopRun()) {
        std::cerr << "Noop run for Torch L0 benchmark" << std::endl;
        profiler.pushNoop(statistics);
        return TestResult::Nooped;
    }

    // setup
    constexpr bool useOoq = false;
    Sycl sycl{sycl::device{sycl::gpu_selector_v}, useOoq};

    data_type *d_out = sycl::malloc_device<data_type>(1, sycl.queue);
    if (d_out == nullptr) {
        return TestResult::Error;
    }

    // warmup
    for (int i = 0; i < 2; i++) {
        ASSERT_TEST_RESULT_SUCCESS(submit_kernel_linear_kernel_size(sycl.queue, args.kernelSize, d_out));
    }
    sycl.queue.wait();

    // benchmarking
    for (size_t i = 0; i < args.iterations; ++i) {
        profiler.measureStart();
        ASSERT_TEST_RESULT_SUCCESS(submit_kernel_linear_kernel_size(sycl.queue, args.kernelSize, d_out));
        profiler.measureEnd();
        profiler.pushStats(statistics);

        // expect a wait here after a batch of submissions, if batch > 0
        if (args.kernelBatchSize > 0 && ((i + 1) % args.kernelBatchSize) == 0) {
            sycl.queue.wait();
        }
    }
    sycl.queue.wait();

    // verify and clean up
    data_type *host_data = sycl::malloc_host<data_type>(1, sycl.queue);
    if (!host_data) {
        sycl::free(d_out, sycl.queue);
        return TestResult::Error;
    }
    sycl.queue.memcpy(host_data, d_out, sizeof(data_type)).wait();
    if (host_data[0] > (static_cast<data_type>(args.kernelSize) + 0.1) || host_data[0] < (static_cast<data_type>(args.kernelSize) - 0.1)) {
        std::cout << "Wrong checker value: " << host_data[0] << ", expected " << static_cast<data_type>(args.kernelSize) << std::endl;
        sycl::free(d_out, sycl.queue);
        sycl::free(host_data, sycl.queue);
        return TestResult::Error;
    }

    sycl::free(d_out, sycl.queue);
    sycl::free(host_data, sycl.queue);

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<KernelSubmitLinearKernelSize> registerTestCase(run, Api::SYCL, false);
