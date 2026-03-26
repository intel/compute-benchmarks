/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "common.hpp"
#include "definitions/kernel_submit_event_record_wait.h"

static TestResult run(const KernelSubmitEventRecordWaitArguments &args, Statistics &statistics) {
    ComboProfilerWithStats profiler(Configuration::get().profilerType);

    if (isNoopRun()) {
        profiler.pushNoop(statistics);
        return TestResult::Nooped;
    }

    // setup
    ExtensionProperties extensionProperties = ExtensionProperties::create();
    LevelZero l0{extensionProperties};
    CommandList cmdListImmediate_1{l0.context, l0.device, zeDefaultGPUImmediateCommandQueueDesc};
    CommandList cmdListImmediate_2{l0.context, l0.device, zeDefaultGPUImmediateCommandQueueDesc};

    // create kernel
    std::string kernelFileName = "torch_benchmark_elementwise_sum_0.cl";
    std::string kernelName = "elementwise_sum_0";
    Kernel kernel_empty{l0, kernelFileName, kernelName};

    const uint32_t wgc = args.kernelWGCount;
    const uint32_t wgs = args.kernelWGSize;
    ze_group_count_t dispatch{wgc, 1, 1};
    ze_group_size_t groupSizes{wgs, 1, 1};

    // benchmark
    for (size_t i = 0; i < args.iterations; ++i) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(cmdListImmediate_1.get(), kernel_empty.get(), dispatch, groupSizes, nullptr, nullptr, nullptr, 0, nullptr));

        profiler.measureStart();

        CounterBasedEvent event_q{l0, args.useProfiling};
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendSignalEvent(cmdListImmediate_1.get(), event_q.get()));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendWaitOnEvents(cmdListImmediate_2.get(), 1, event_q.getAddress()));

        profiler.measureEnd();
        profiler.pushStats(statistics);

        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(cmdListImmediate_2.get(), kernel_empty.get(), dispatch, groupSizes, nullptr, nullptr, nullptr, 0, nullptr));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdListImmediate_1.get(), UINT64_MAX));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdListImmediate_2.get(), UINT64_MAX));

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<KernelSubmitEventRecordWait> registerTestCase(run, Api::L0, true);
