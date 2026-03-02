/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "common.hpp"
#include "definitions/kernel_submit_single_queue.h"

constexpr size_t pool_double_size = 2;
constexpr size_t pool_int_size = 3;

template <typename T>
using DevicePtr = decltype(make_device_ptr<T>(std::declval<Sycl &>(), std::declval<size_t>()));
template <typename T>
using HostPtr = decltype(make_host_ptr<T>(std::declval<Sycl &>(), std::declval<size_t>()));

template <typename T>
static void initializeCopyableObjectDevice(DevicePtr<T> &buffer, size_t count, std::vector<DevicePtr<double>> &pool_double, std::vector<DevicePtr<int>> &pool_int, Sycl &sycl) {
    if constexpr (std::is_same_v<T, CopyableObject>) {
        for (size_t i = 0; i < count; i++) {
            auto alloc_d = make_device_ptr<double>(sycl, pool_double_size);
            auto alloc_i = make_device_ptr<int>(sycl, pool_int_size);
            pool_double.push_back(std::move(alloc_d));
            pool_int.push_back(std::move(alloc_i));
            CopyableObject data(static_cast<float>(i), alloc_d.get(), alloc_i.get(), pool_double_size, pool_int_size);
            sycl.queue.memcpy(&buffer.get()[i], &data, sizeof(CopyableObject))
                .wait();
        }
    }
}

template <typename T>
static void initializeCopyableObjectHost(HostPtr<T> &buffer, size_t count, std::vector<HostPtr<double>> &pool_double, std::vector<HostPtr<int>> &pool_int, Sycl &sycl) {
    if constexpr (std::is_same_v<T, CopyableObject>) {
        for (size_t i = 0; i < count; ++i) {
            auto alloc_d = make_host_ptr<double>(sycl, pool_double_size);
            auto alloc_i = make_host_ptr<int>(sycl, pool_int_size);
            buffer.get()[i].p2 = alloc_d.get();
            buffer.get()[i].p3 = alloc_i.get();
            pool_double.push_back(std::move(alloc_d));
            pool_int.push_back(std::move(alloc_i));
        }
    }
}

template <typename T>
static DevicePtr<T> allocateBufferDevice(size_t bufferSize,
                                         std::vector<DevicePtr<double>> &pool_double, std::vector<DevicePtr<int>> &pool_int,
                                         Sycl &sycl) {
    DevicePtr<T> deviceBuffer = make_device_ptr<T>(sycl, bufferSize);
    initializeCopyableObjectDevice<T>(deviceBuffer, bufferSize, pool_double, pool_int, sycl);
    return deviceBuffer;
}

template <typename T>
static HostPtr<T> allocateBufferHost(size_t bufferSize, std::vector<HostPtr<double>> &pool_double, std::vector<HostPtr<int>> &pool_int,
                                     Sycl &sycl) {
    HostPtr<T> hostBuffer = make_host_ptr<T>(sycl, bufferSize);
    initializeCopyableObjectHost<T>(hostBuffer, bufferSize, pool_double, pool_int, sycl);
    return hostBuffer;
}

template <typename T>
static TestResult runBenchmark(const KernelSubmitSingleQueueArguments &args, ComboProfilerWithStats &profiler, Statistics &statistics) {
    // setup
    constexpr bool useOoq = false;
    Sycl sycl{sycl::device{sycl::gpu_selector_v}, useOoq};
    const size_t bufferSize = args.kernelWGCount * args.kernelWGSize;

    // benchmark an empty kernel, no allocations need to be made
    if (args.kernelName == KernelName::Empty) {
        for (size_t i = 0; i < args.iterations; i++) {
            profiler.measureStart();
            submit_kernel_empty(args.kernelWGCount, args.kernelWGSize, sycl.queue, args.useEvents);
            profiler.measureEnd();
            profiler.pushStats(statistics);

            if (args.kernelBatchSize > 0 && (i + 1) % args.kernelBatchSize == 0) {
                sycl.queue.wait();
            }
        }
        sycl.queue.wait();
        return TestResult::Success;
    }

    // allocate and initialize buffers based on args.kernelDataType type
    std::vector<DevicePtr<double>> pool_double;
    std::vector<DevicePtr<int>> pool_int;

    unsigned num_main_buffers = args.kernelParamsNum;
    unsigned num_float_buffers = 0;
    unsigned num_int_buffers = 0;
    if (args.kernelDataType == DataType::Mixed) {
        // The kernel used in this scenario requires these fixed sizes
        num_main_buffers = 3;
        num_float_buffers = 4;
        num_int_buffers = 3;
    }

    DevicePtr<T> deviceBuffer = allocateBufferDevice<T>(bufferSize, pool_double, pool_int, sycl);

    std::vector<DevicePtr<T>> deviceBufferVec;
    deviceBufferVec.reserve(num_main_buffers);
    for (unsigned i = 0; i < num_main_buffers; i++) {
        deviceBufferVec.push_back(allocateBufferDevice<T>(bufferSize, pool_double, pool_int, sycl));
    }
    // Extract raw pointers for kernel submission
    std::vector<T *> rawPtrs;
    rawPtrs.reserve(num_main_buffers);
    for (unsigned i = 0; i < num_main_buffers; i++) {
        rawPtrs.push_back(deviceBufferVec[i].get());
    }

    std::vector<DevicePtr<float>> floatDeviceBufferVec;
    std::vector<DevicePtr<int>> intDeviceBufferVec;
    if (args.kernelDataType == DataType::Mixed) {
        floatDeviceBufferVec.reserve(num_float_buffers);
        intDeviceBufferVec.reserve(num_int_buffers);
        for (unsigned i = 0; i < num_float_buffers; i++) {
            floatDeviceBufferVec.push_back(allocateBufferDevice<float>(bufferSize, pool_double, pool_int, sycl));
        }
        for (unsigned i = 0; i < num_int_buffers; i++) {
            intDeviceBufferVec.push_back(allocateBufferDevice<int>(bufferSize, pool_double, pool_int, sycl));
        }
    }

    std::optional<HostPtr<T>> hostBuffer;
    if (args.kernelSubmitPattern == KernelSubmitPattern::H2d_before_batch ||
        args.kernelSubmitPattern == KernelSubmitPattern::D2h_after_batch) {
        hostBuffer.emplace(allocateBufferHost<T>(bufferSize, pool_double, pool_int, sycl));
    }

    // benchmark
    for (size_t i = 0; i < args.iterations; i++) {
        if (args.kernelSubmitPattern == KernelSubmitPattern::H2d_before_batch) {
            // Host to device copy before each batch
            sycl.queue.memcpy(deviceBuffer.get(), hostBuffer->get(), bufferSize * sizeof(T));
        }

        profiler.measureStart();
        if (args.kernelDataType == DataType::Mixed) {
            if constexpr (std::is_same<T, double>::value) {
                submit_kernel_add_mixed_type<double, float, int>(args.kernelWGCount, args.kernelWGSize, sycl.queue, args.useEvents, deviceBuffer.get(), deviceBufferVec[0].get(), deviceBufferVec[1].get(), deviceBufferVec[2].get(),
                                                                 floatDeviceBufferVec[0].get(), floatDeviceBufferVec[1].get(), floatDeviceBufferVec[2].get(), floatDeviceBufferVec[3].get(),
                                                                 intDeviceBufferVec[0].get(), intDeviceBufferVec[1].get(), intDeviceBufferVec[2].get());
            }
        } else {
            submit_kernel_add<T>(args.kernelWGCount, args.kernelWGSize, sycl.queue, args.useEvents,
                                 deviceBuffer.get(), rawPtrs.data(), num_main_buffers);
        }
        profiler.measureEnd();
        profiler.pushStats(statistics);

        if (args.kernelBatchSize > 0 && (i + 1) % args.kernelBatchSize == 0) {
            sycl.queue.wait();
            if (args.kernelSubmitPattern == KernelSubmitPattern::D2h_after_batch) {
                // Device to host copy after each batch
                sycl.queue.memcpy(hostBuffer->get(), deviceBuffer.get(), bufferSize * sizeof(T));
            }
        }
    }
    sycl.queue.wait();
    // Final D2H copy if needed (for last batch that didn't hit boundary)
    if (args.kernelSubmitPattern == KernelSubmitPattern::D2h_after_batch) {
        sycl.queue.memcpy(hostBuffer->get(), deviceBuffer.get(), bufferSize * sizeof(T));
    }

    return TestResult::Success;
}

static TestResult run(const KernelSubmitSingleQueueArguments &args, Statistics &statistics) {
    ComboProfilerWithStats profiler(Configuration::get().profilerType);

    if (isNoopRun()) {
        profiler.pushNoop(statistics);
        return TestResult::Nooped;
    }

    // validate arguments
    // Currently limit args.kernelParamsNum up to 10
    if (args.kernelParamsNum < 1u) {
        std::cerr << "kernelParamsNum must be at least 1" << std::endl;
        return TestResult::InvalidArgs;
    } else if (args.kernelParamsNum > 10u) {
        std::cerr << "kernelParamsNum must be at most 10" << std::endl;
        return TestResult::InvalidArgs;
    }

    switch (args.kernelDataType) {
    case DataType::Int32:
        ASSERT_TEST_RESULT_SUCCESS(runBenchmark<int>(args, profiler, statistics));
        break;
    case DataType::Float:
        ASSERT_TEST_RESULT_SUCCESS(runBenchmark<float>(args, profiler, statistics));
        break;
    case DataType::Double:
    case DataType::Mixed:
        ASSERT_TEST_RESULT_SUCCESS(runBenchmark<double>(args, profiler, statistics));
        break;
    case DataType::CopyableObject:
        ASSERT_TEST_RESULT_SUCCESS(runBenchmark<CopyableObject>(args, profiler, statistics));
        break;
    default:
        return TestResult::InvalidArgs;
    }

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<KernelSubmitSingleQueue> registerTestCase(run, Api::SYCL);
