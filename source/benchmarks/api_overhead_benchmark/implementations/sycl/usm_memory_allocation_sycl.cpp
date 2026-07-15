/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/sycl/sycl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/combo_profiler.h"

#include "definitions/usm_memory_allocation.h"

#if __has_include(<sycl/ext/oneapi/experimental/async_alloc/async_alloc.hpp>)
#include <sycl/ext/oneapi/experimental/async_alloc/async_alloc.hpp>
#define HAS_SYCL_ASYNC_ALLOC 1
#else
#define HAS_SYCL_ASYNC_ALLOC 0
#endif

static auto inOrder = sycl::property::queue::in_order();

static bool hasUsmPlacementSupport(const sycl::device &device, UsmRuntimeMemoryPlacement placement) {
    switch (placement) {
    case UsmRuntimeMemoryPlacement::Host:
        return device.has(sycl::aspect::usm_host_allocations);
    case UsmRuntimeMemoryPlacement::Device:
        return device.has(sycl::aspect::usm_device_allocations);
    case UsmRuntimeMemoryPlacement::Shared:
        return device.has(sycl::aspect::usm_shared_allocations);
    default:
        return false;
    }
}

static TestResult run(const UsmMemoryAllocationArguments &arguments, Statistics &statistics) {
    ComboProfilerWithStats profiler(Configuration::get().profilerType);

    if (isNoopRun()) {
        profiler.pushNoop(statistics);
        return TestResult::Nooped;
    }

    // Setup
    Sycl sycl = Sycl(inOrder);
    bool isAsync = false;
    void *ptr{};

    switch (arguments.strategy) {
    case MemoryStrategy::Sync:
        isAsync = false;
        break;
    case MemoryStrategy::Async:
#if HAS_SYCL_ASYNC_ALLOC
        if (!sycl.device.has(sycl::aspect::ext_oneapi_async_memory_alloc)) {
            std::cerr << "Async SYCL USM allocations are not supported!" << std::endl;
            return TestResult::DeviceNotCapable;
        }
#else
        std::cerr << "SYCL async allocation experimental extension header is unavailable" << std::endl;
        return TestResult::DeviceNotCapable;
#endif
        isAsync = true;
        break;
    default:
        std::cerr << "use either Sync or Async memory strategy" << std::endl;
        return TestResult::InvalidArgs;
    }

    // check if proper memory placement was picked and make sure
    // that if async memory strategy is selected, it is for Device memory placement
    if (arguments.usmMemoryPlacement != UsmRuntimeMemoryPlacement::Device) {
        if (arguments.usmMemoryPlacement != UsmRuntimeMemoryPlacement::Host &&
            arguments.usmMemoryPlacement != UsmRuntimeMemoryPlacement::Shared) {
            std::cerr << "use either Host, Device or Shared memory placement" << std::endl;
            return TestResult::InvalidArgs;
        } else if (isAsync) {
            std::cerr << "Async memory strategy is only supported for Device memory placement" << std::endl;
            return TestResult::InvalidArgs;
        }
    }

    // check if USM support is available on the requested placement type
    if (!hasUsmPlacementSupport(sycl.device, arguments.usmMemoryPlacement)) {
        std::cerr << "USM allocations are not supported!" << std::endl;
        return TestResult::DeviceNotCapable;
    }

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        if (arguments.measureMode == AllocationMeasureMode::Allocate ||
            arguments.measureMode == AllocationMeasureMode::Both) {
            profiler.measureStart();
        }

        if (isAsync) {
#if HAS_SYCL_ASYNC_ALLOC
            // SYCL only supports async memory strategy for device placement
            auto allocKind = sycl::usm::alloc::device;
            ptr = sycl::ext::oneapi::experimental::async_malloc(sycl.queue, allocKind, arguments.size);
#endif
        } else {
            switch (arguments.usmMemoryPlacement) {
            case UsmRuntimeMemoryPlacement::Host:
                ptr = sycl::malloc_host(arguments.size, sycl.queue);
                break;
            case UsmRuntimeMemoryPlacement::Device:
                ptr = sycl::malloc_device(arguments.size, sycl.queue);
                break;
            case UsmRuntimeMemoryPlacement::Shared:
                ptr = sycl::malloc_shared(arguments.size, sycl.queue);
                break;
            default:
                // unreachable, as we have already checked for valid placements above
                break;
            }
        }

        if (arguments.measureMode == AllocationMeasureMode::Allocate) {
            profiler.measureEnd();
        } else if (arguments.measureMode == AllocationMeasureMode::Free) {
            profiler.measureStart();
        }

        if (isAsync) {
#if HAS_SYCL_ASYNC_ALLOC
            sycl::ext::oneapi::experimental::async_free(sycl.queue, ptr);
#endif
        } else {
            sycl::free(ptr, sycl.queue);
        }

        if (arguments.measureMode == AllocationMeasureMode::Free ||
            arguments.measureMode == AllocationMeasureMode::Both) {
            profiler.measureEnd();
        }

        profiler.pushStats(statistics);
        if (isAsync) {
            sycl.queue.wait();
        }
    }

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<UsmMemoryAllocation> registerTestCase(run, Api::SYCL);
