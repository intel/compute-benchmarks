/*
 * Copyright (C) 2023-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"
#include "framework/utility/combo_profiler.h"

#include "definitions/kernel_submit_multi_queue.h"
#include "definitions/sycl_kernels.h"

#include <gtest/gtest.h>
#include <sycl/sycl.hpp>

#define NUM_OF_QUEUES 2

using data_type = int;

static TestResult run(const KernelSubmitMultiQueueArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // sycl::device dev = sycl::device([](const sycl::device&){return 0;}); // use it to run on CPU
    sycl::device dev = sycl::device(sycl::gpu_selector_v);

    assert(NUM_OF_QUEUES == 2);

    sycl::queue q[NUM_OF_QUEUES];
    for (int i = 0; i < NUM_OF_QUEUES; i++) {
        q[i] = sycl::queue(dev, sycl::property::queue::in_order());
    }

    data_type *d_a[NUM_OF_QUEUES];
    data_type *d_b[NUM_OF_QUEUES];
    data_type *d_c[NUM_OF_QUEUES];

    for (int i = 0; i < NUM_OF_QUEUES; i++) {
        const size_t data_size = arguments.workgroupCount * arguments.workgroupSize;
        d_a[i] = sycl::malloc_device<data_type>(data_size, q[i]);
        d_b[i] = sycl::malloc_device<data_type>(data_size, q[i]);
        d_c[i] = sycl::malloc_device<data_type>(data_size, q[i]);
    }

    for (int i = 0; i < NUM_OF_QUEUES; i++) {
        q[i].wait();
    }

    // Totally submit numIterations of a specific kernel
    for (size_t i = 0; i < arguments.iterations; i++) {
        // Submit several kernels into queue1
        for (int j = 0; j < arguments.kernelsPerQueue; j++) {
            submit_kernel_add<data_type>(arguments.workgroupCount, arguments.workgroupSize, q[0], d_a[0], d_b[0], d_c[0]);
        }

        // Submit several kernels into queue2
        for (int j = 0; j < arguments.kernelsPerQueue - 1; j++) {
            submit_kernel_add<data_type>(arguments.workgroupCount, arguments.workgroupSize, q[1], d_a[1], d_b[1], d_c[1]);
        }
        // q2_last_event is the last event of queue2
        sycl::event q2_last_event = submit_with_event_kernel_add<data_type>(arguments.workgroupCount, arguments.workgroupSize, q[1], d_a[1], d_b[1], d_c[1]);

        // Start to measure submit time for a specific kernel
        Timer timer;
        timer.measureStart();
        // Submit a new kernel into queue1, but the new kernel can only be executed after q2_last_event ends
        submit_kernel_add<data_type>(arguments.workgroupCount, arguments.workgroupSize, q[0], q2_last_event, d_a[0], d_b[0], d_c[0]);
        timer.measureEnd();

        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    for (int i = 0; i < NUM_OF_QUEUES; i++) {
        q[i].wait();
    }

    for (int i = 0; i < NUM_OF_QUEUES; i++) {
        if (d_a[i] != NULL)
            sycl::free(d_a[i], q[i]);
        if (d_b[i] != NULL)
            sycl::free(d_b[i], q[i]);
        if (d_c[i] != NULL)
            sycl::free(d_c[i], q[i]);
    }

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<KernelSubmitMultiQueue> registerTestCase(run, Api::SYCL);
