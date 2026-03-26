/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "common.hpp"
#include "definitions/kernel_submit_event_record_query.h"

static TestResult run(const KernelSubmitEventRecordQueryArguments &args, Statistics &statistics) {
    ComboProfilerWithStats profiler(Configuration::get().profilerType);

    if (isNoopRun()) {
        profiler.pushNoop(statistics);
        return TestResult::Nooped;
    }

    // setup
    ExtensionProperties extensionProperties = ExtensionProperties::create();
    LevelZero l0{extensionProperties};
    CommandList cmdListImmediate{l0.context, l0.device, zeDefaultGPUImmediateCommandQueueDesc};

    // create kernel
    std::string kernelFileName = "torch_benchmark_elementwise_sum_0.cl";
    std::string kernelName = "elementwise_sum_0";
    Kernel kernelEmpty{l0, kernelFileName, kernelName};

    const uint32_t wgc = args.kernelWGCount;
    const uint32_t wgs = args.kernelWGSize;
    ze_group_count_t dispatch{wgc, 1, 1};
    ze_group_size_t groupSizes{wgs, 1, 1};

    // benchmark
    for (size_t i = 0; i < args.iterations; ++i) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(cmdListImmediate.get(), kernelEmpty.get(), dispatch, groupSizes, nullptr, nullptr, nullptr, 0, nullptr));

        CounterBasedEvent event{l0, args.useProfiling};
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendSignalEvent(cmdListImmediate.get(), event.get()));

        ze_result_t status = zeEventHostSynchronize(event.get(), UINT64_MAX);
        ASSERT_ZE_RESULT_SUCCESS(status);
        // Ensure the event is completed before querying
        status = zeEventQueryStatus(event.get());
        ASSERT_ZE_RESULT_SUCCESS(status);

        profiler.measureStart();
        for (uint32_t query = 0; query < args.eventQueryIterations; ++query) {
            status = zeEventQueryStatus(event.get());
        }
        profiler.measureEnd();
        profiler.pushStats(statistics);

        ASSERT_ZE_RESULT_SUCCESS(status);
    }

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<KernelSubmitEventRecordQuery> registerTestCase(run, Api::L0, true);
