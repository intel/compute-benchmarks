/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/sycl/sycl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/combo_profiler.h"

#include "definitions/kernel_submit_single_queue.h"
#include "definitions/sycl_kernels.h"

#include <gtest/gtest.h>

constexpr size_t pool_double_size = 2;
constexpr size_t pool_int_size = 3;

template <typename T>
static void initializeCopyableObjectDevice(T *buffer, size_t count, std::vector<double *> &pool_double, std::vector<int *> &pool_int, sycl::queue &queue) {
    if constexpr (std::is_same_v<T, CopyableObject>) {
        std::vector<CopyableObject> device_data(count);
        for (size_t i = 0; i < count; i++) {
            double *alloc_d = sycl::malloc_device<double>(pool_double_size, queue);
            int *alloc_i = sycl::malloc_device<int>(pool_int_size, queue);
            pool_double.push_back(alloc_d);
            pool_int.push_back(alloc_i);
            device_data.push_back(CopyableObject(static_cast<float>(i), alloc_d, alloc_i, pool_double_size, pool_int_size));
        }
        queue.memcpy(buffer, device_data.data(), count * sizeof(CopyableObject))
            .wait();
    }
}

template <typename T>
static void initializeCopyableObjectHost(T *buffer, size_t count, std::vector<double *> &pool_double, std::vector<int *> &pool_int, sycl::queue &queue) {
    if constexpr (std::is_same_v<T, CopyableObject>) {
        for (size_t i = 0; i < count; ++i) {
            buffer[i].p2 = sycl::malloc_host<double>(pool_double_size, queue);
            buffer[i].p3 = sycl::malloc_host<int>(pool_int_size, queue);
            pool_double.push_back(buffer[i].p2);
            pool_int.push_back(buffer[i].p3);
        }
    }
}

template <typename T>
static void allocateBufferDevice(size_t bufferSize,
                                 std::vector<double *> &pool_double, std::vector<int *> &pool_int,
                                 sycl::queue &queue, T *&deviceBuffer) {
    deviceBuffer = sycl::malloc_device<T>(bufferSize, queue);
    initializeCopyableObjectDevice(deviceBuffer, bufferSize, pool_double, pool_int, queue);
}

template <typename T>
static void allocateBufferHost(size_t bufferSize, std::vector<double *> &pool_double, std::vector<int *> &pool_int,
                               sycl::queue &queue, T *&hostBuffer) {
    hostBuffer = sycl::malloc_host<T>(bufferSize, queue);
    initializeCopyableObjectHost(hostBuffer, bufferSize, pool_double, pool_int, queue);
}

template <typename T>
static TestResult runBenchmark(const KernelSubmitSingleQueueArguments &args, ComboProfilerWithStats &profiler, Statistics &statistics) {
    // Setup
    constexpr bool useOoq = false;
    Sycl sycl{sycl::device{sycl::gpu_selector_v}, useOoq};

    const size_t bufferSize = args.kernelWGCount * args.kernelWGSize;

    // Allocate and initialize buffers based on args.kernelDataType type
    T *deviceBuffer = nullptr;
    T *hostBuffer = nullptr;
    std::vector<T *> deviceBufferVec;
    std::vector<float *> floatDeviceBufferVec;
    std::vector<int *> intDeviceBufferVec;
    std::vector<double *> pool_double;
    std::vector<int *> pool_int;
    // Currently limit args.kernelParamsNum up to 10
    if (args.kernelParamsNum < 1u) {
        std::cerr << "kernelParamsNum must be at least 1" << std::endl;
        return TestResult::InvalidArgs;
    } else if (args.kernelParamsNum > 10u) {
        std::cerr << "kernelParamsNum must be at most 10" << std::endl;
        return TestResult::InvalidArgs;
    }
    unsigned num_main_buffers = args.kernelParamsNum;

    unsigned num_float_buffers = 0;
    unsigned num_int_buffers = 0;
    if (args.kernelDataType == DataType::Mixed) {
        // The kernel used in this scenario requires these fixed sizes
        num_main_buffers = 3;
        num_float_buffers = 4;
        num_int_buffers = 3;
    }

    if (args.kernelName != KernelName::Empty) {
        allocateBufferDevice<T>(bufferSize, pool_double, pool_int, sycl.queue, deviceBuffer);
        deviceBufferVec.resize(num_main_buffers);
        for (unsigned i = 0; i < num_main_buffers; i++) {
            allocateBufferDevice<T>(bufferSize, pool_double, pool_int, sycl.queue, deviceBufferVec[i]);
        }
        if (args.kernelDataType == DataType::Mixed) {
            floatDeviceBufferVec.resize(num_float_buffers);
            intDeviceBufferVec.resize(num_int_buffers);
            for (unsigned i = 0; i < num_float_buffers; i++) {
                allocateBufferDevice<float>(bufferSize, pool_double, pool_int, sycl.queue, floatDeviceBufferVec[i]);
            }
            for (unsigned i = 0; i < num_int_buffers; i++) {
                allocateBufferDevice<int>(bufferSize, pool_double, pool_int, sycl.queue, intDeviceBufferVec[i]);
            }
        }
        if (args.kernelSubmitPattern == KernelSubmitPattern::H2d_before_batch ||
            args.kernelSubmitPattern == KernelSubmitPattern::D2h_after_batch) {
            allocateBufferHost<T>(bufferSize, pool_double, pool_int, sycl.queue, hostBuffer);
        }
    }

    // Totally submit iterations of a specific kernel, can be grouped in several batches,
    // each batch submits batch_size of kernels, then followed by a queue.wait.
    for (size_t i = 0; i < args.iterations; i++) {
        if (args.kernelSubmitPattern == KernelSubmitPattern::H2d_before_batch && args.kernelName != KernelName::Empty) {
            // Host to device copy before each batch
            sycl.queue.memcpy(deviceBuffer, hostBuffer, bufferSize * sizeof(T));
        }
        // The measured kernel submission
        profiler.measureStart();
        if (args.kernelName == KernelName::Empty) {
            submit_kernel_empty(args.kernelWGCount, args.kernelWGSize, sycl.queue, args.useEvents);
        } else if (args.kernelDataType == DataType::Mixed) {
            if constexpr (std::is_same<T, double>::value) {
                submit_kernel_add_mixed_type<double, float, int>(args.kernelWGCount, args.kernelWGSize, sycl.queue, args.useEvents,
                                                                 deviceBuffer, deviceBufferVec[0], deviceBufferVec[1], deviceBufferVec[2],
                                                                 floatDeviceBufferVec[0], floatDeviceBufferVec[1], floatDeviceBufferVec[2], floatDeviceBufferVec[3],
                                                                 intDeviceBufferVec[0], intDeviceBufferVec[1], intDeviceBufferVec[2]);
            }
        } else {
            submit_kernel_add<T>(args.kernelWGCount, args.kernelWGSize, sycl.queue, args.useEvents,
                                 deviceBuffer, deviceBufferVec.data(), num_main_buffers);
        }
        profiler.measureEnd();
        profiler.pushStats(statistics);

        if (args.kernelSubmitPattern == KernelSubmitPattern::D2h_after_batch && args.kernelName != KernelName::Empty) {
            // Device to host copy after each batch
            sycl.queue.memcpy(hostBuffer, deviceBuffer, bufferSize * sizeof(T));
        }

        if (args.kernelBatchSize > 0 && (i + 1) % args.kernelBatchSize == 0) {
            sycl.queue.wait();
        }
    }
    sycl.queue.wait();

    if (args.kernelName == KernelName::Empty) {
        // No device/host allocations to free
        return TestResult::Success;
    }

    // Cleanup all directly allocated memory
    sycl::free(deviceBuffer, sycl.queue);
    sycl::free(hostBuffer, sycl.queue);

    // Cleanup device vectors buffers
    for (auto *ptr : deviceBufferVec) {
        sycl::free(ptr, sycl.queue);
    }
    for (auto *ptr : floatDeviceBufferVec) {
        sycl::free(ptr, sycl.queue);
    }
    for (auto *ptr : intDeviceBufferVec) {
        sycl::free(ptr, sycl.queue);
    }

    // Free all allocated double and int pointers
    for (auto *ptr : pool_double) {
        sycl::free(ptr, sycl.queue);
    }
    for (auto *ptr : pool_int) {
        sycl::free(ptr, sycl.queue);
    }
    return TestResult::Success;
}

static TestResult run(const KernelSubmitSingleQueueArguments &args, Statistics &statistics) {
    ComboProfilerWithStats profiler(Configuration::get().profilerType);

    if (isNoopRun()) {
        profiler.pushNoop(statistics);
        return TestResult::Nooped;
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
