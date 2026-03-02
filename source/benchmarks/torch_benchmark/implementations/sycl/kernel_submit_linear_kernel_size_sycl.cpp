/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "common.hpp"
#include "definitions/kernel_submit_linear_kernel_size.h"
#include "definitions/linear_kernel_size_kernels.h"

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
        profiler.pushNoop(statistics);
        return TestResult::Nooped;
    }

    // setup
    bool useOoq = false;
    Sycl sycl = args.useProfiling
                    ? Sycl{useOoq, sycl::property::queue::enable_profiling()}
                    : Sycl{useOoq};
    auto d_out = make_device_ptr<data_type>(sycl, 1);

    // benchmark
    for (size_t i = 0; i < args.iterations; ++i) {
        profiler.measureStart();
        ASSERT_TEST_RESULT_SUCCESS(submit_kernel_linear_kernel_size(sycl.queue, args.kernelSize, d_out.get()));
        profiler.measureEnd();
        profiler.pushStats(statistics);

        // expect a wait here after a batch of submissions, if batch > 0
        if (args.kernelBatchSize > 0 && ((i + 1) % args.kernelBatchSize) == 0) {
            sycl.queue.wait();
        }
    }
    sycl.queue.wait();

    // verify result
    auto host_data = make_host_ptr<data_type>(sycl, 1);
    sycl.queue.memcpy(host_data.get(), d_out.get(), sizeof(data_type)).wait();
    if (*host_data > (static_cast<data_type>(args.kernelSize) + 0.1) || *host_data < (static_cast<data_type>(args.kernelSize) - 0.1)) {
        std::cout << "Wrong checker value: " << *host_data << ", expected " << static_cast<data_type>(args.kernelSize) << std::endl;
        return TestResult::Error;
    }

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<KernelSubmitLinearKernelSize> registerTestCase(run, Api::SYCL, false);
