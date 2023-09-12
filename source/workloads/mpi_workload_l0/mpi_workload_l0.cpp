/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/enum/mpi_test.h"
#include "framework/enum/usm_memory_placement.h"
#include "framework/l0/levelzero.h"
#include "framework/l0/utility/error_codes.h"
#include "framework/workload/register_workload.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <mpi.h>
#include <numeric>

#define ZECHK(ZECALL)                                                      \
    {                                                                      \
        const ze_result_t zeRet = (ZECALL);                                \
        if (zeRet != ZE_RESULT_SUCCESS) {                                  \
            std::cerr << "**ERROR: " << __FILE__ << ':' << __LINE__ << ' ' \
                      << __PRETTY_FUNCTION__ << " | " << #ZECALL << " -> " \
                      << l0ErrorToString(zeRet) << std::endl;              \
        }                                                                  \
    }

#define MPICHK(MPICALL)                                                     \
    {                                                                       \
        const int mpiErrno = (MPICALL);                                     \
        if (mpiErrno != MPI_SUCCESS) {                                      \
            std::cerr << "**ERROR: " << __FILE__ << ':' << __LINE__ << ' '  \
                      << __PRETTY_FUNCTION__ << " | " << #MPICALL << " -> " \
                      << mpiErrno << std::endl;                             \
        }                                                                   \
    }

struct MpiArguments : WorkloadArgumentContainer {
    IntegerArgument testType;
    IntegerArgument statsType;
    IntegerArgument testArg0;
    IntegerArgument testArg1;
    IntegerArgument testArg2;
    IntegerArgument testArg3;
    IntegerArgument testArg4;

    MpiArguments()
        : testType(*this, "testType", "Type of the test"),
          statsType(*this, "statsType", "Type of the statistics"),
          testArg0(*this, "testArg0", "Test-specific argument 0"),
          testArg1(*this, "testArg1", "Test-specific argument 1"),
          testArg2(*this, "testArg2", "Test-specific argument 2"),
          testArg3(*this, "testArg3", "Test-specific argument 3"),
          testArg4(*this, "testArg4", "Test-specific argument 4") {
        testType = static_cast<int>(MpiTestType::Startup);
        statsType = static_cast<int>(MpiStatisticsType::Avg);
        testArg0 = 0;
        testArg1 = 0;
        testArg2 = 0;
        testArg3 = 0;
        testArg4 = 0;
    }
};

struct MpiWorkload : Workload<MpiArguments> {};

uint64_t getTimeDiffNs(const timespec &t_start, const timespec &t_end) {
    return (t_end.tv_sec - t_start.tv_sec) * 1000000000u + (t_end.tv_nsec - t_start.tv_nsec);
}

void *memAlloc(LevelZero &levelzero, const int type, const size_t size, const size_t alignment) {
    void *ptr = nullptr;

    ze_host_mem_alloc_desc_t hostBufferDesc{};
    hostBufferDesc.stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC;
    hostBufferDesc.pNext = nullptr;
    hostBufferDesc.flags = ZE_HOST_MEM_ALLOC_FLAG_BIAS_UNCACHED;

    ze_device_mem_alloc_desc_t deviceBufferDesc{};
    deviceBufferDesc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;
    deviceBufferDesc.pNext = nullptr;
    deviceBufferDesc.flags = ZE_DEVICE_MEM_ALLOC_FLAG_BIAS_UNCACHED;
    deviceBufferDesc.ordinal = 0;

    const UsmMemoryPlacement memoryType = static_cast<UsmMemoryPlacement>(type);
    switch (memoryType) {
    case UsmMemoryPlacement::Host: {
        ZECHK(zeMemAllocHost(levelzero.context, &hostBufferDesc, size, alignment, &ptr));
        break;
    }
    case UsmMemoryPlacement::Device: {
        ZECHK(zeMemAllocDevice(levelzero.context, &deviceBufferDesc, size, alignment, levelzero.device, &ptr));
        break;
    }
    case UsmMemoryPlacement::Shared: {
        ZECHK(zeMemAllocShared(levelzero.context, &deviceBufferDesc, &hostBufferDesc, size, alignment, levelzero.device, &ptr));
        break;
    }
    default: {
        ptr = aligned_alloc(alignment, (size % alignment == 0) ? size : (size / alignment + 1) * alignment);
        break;
    }
    }
    EXPECT_NE(nullptr, ptr);
    return ptr;
}

void memFree(LevelZero &levelzero, const int type, void *ptr) {
    const UsmMemoryPlacement memoryType = static_cast<UsmMemoryPlacement>(type);
    if ((memoryType == UsmMemoryPlacement::Host) || (memoryType == UsmMemoryPlacement::Device) || (memoryType == UsmMemoryPlacement::Shared)) {
        ZECHK(zeMemFree(levelzero.context, ptr));
    } else {
        free(ptr);
    }
}

TestResult testStartup(const MpiArguments &arguments, Statistics &statistics, const MpiStatisticsType statsType) {
    const bool initMpiFirst = arguments.testArg0;

    ze_init_flags_t flags = 0;
    switch (arguments.testArg1) {
    case 1: {
        flags = ZE_INIT_FLAG_GPU_ONLY;
        break;
    }
    case 2: {
        flags = ZE_INIT_FLAG_VPU_ONLY;
        break;
    }
    default: {
        break;
    }
    }

    timespec t0, t1;

    clock_gettime(CLOCK_MONOTONIC_RAW, &t0);

    if (initMpiFirst) {
        MPICHK(MPI_Init(nullptr, nullptr));
        ZECHK(zeInit(flags));
    } else {
        ZECHK(zeInit(flags));
        MPICHK(MPI_Init(nullptr, nullptr));
    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &t1);

    const uint64_t duration = getTimeDiffNs(t0, t1);

    int myRank, nRanks;
    MPICHK(MPI_Comm_rank(MPI_COMM_WORLD, &myRank));
    MPICHK(MPI_Comm_size(MPI_COMM_WORLD, &nRanks));

    uint64_t measurement = 0;
    if (statsType == MpiStatisticsType::Avg) {
        MPICHK(MPI_Reduce(&duration, &measurement, 1, MPI_UINT64_T, MPI_SUM, 0, MPI_COMM_WORLD));
        measurement /= nRanks;
    } else if (statsType == MpiStatisticsType::Max) {
        MPICHK(MPI_Reduce(&duration, &measurement, 1, MPI_UINT64_T, MPI_MAX, 0, MPI_COMM_WORLD));
    } else if (statsType == MpiStatisticsType::Min) {
        MPICHK(MPI_Reduce(&duration, &measurement, 1, MPI_UINT64_T, MPI_MIN, 0, MPI_COMM_WORLD));
    }
    if (myRank == 0) {
        std::cout << "**Measurement:" << measurement << std::endl;
    }

    MPICHK(MPI_Finalize());

    for (auto i = 0u; i < arguments.iterations; i++) {
        statistics.pushValue(std::chrono::nanoseconds{0}, MeasurementUnit::Unknown, MeasurementType::Unknown);
    }
    return TestResult::Success;
}

TestResult testBandwidth(const MpiArguments &arguments, Statistics &statistics, const MpiStatisticsType statsType) {
    constexpr uint32_t warmup = 10;
    constexpr int batchSize = 100;
    MPI_Request requests[batchSize];

    const uint32_t messageSize = arguments.testArg0;
    const int sendType = arguments.testArg1;
    const int recvType = arguments.testArg2;
    const uint32_t nBatches = arguments.testArg3;

    std::vector<uint64_t> durations(nBatches);

    LevelZero levelzero{};
    MPICHK(MPI_Init(nullptr, nullptr));

    int myRank;
    MPICHK(MPI_Comm_rank(MPI_COMM_WORLD, &myRank));

    void *buf = memAlloc(levelzero, (myRank == 0) ? sendType : recvType, messageSize, 1);

    timespec t0, t1;

    for (uint32_t i = 0; i < nBatches + warmup; i++) {
        MPICHK(MPI_Barrier(MPI_COMM_WORLD));

        clock_gettime(CLOCK_MONOTONIC_RAW, &t0);
        for (int j = 0; j < batchSize; j++) {
            if (myRank == 0) {
                MPICHK(MPI_Isend(buf, messageSize, MPI_INT8_T, 1, 0, MPI_COMM_WORLD, &requests[j]));
            } else {
                MPICHK(MPI_Irecv(buf, messageSize, MPI_INT8_T, 0, 0, MPI_COMM_WORLD, &requests[j]));
            }
        }
        MPICHK(MPI_Waitall(batchSize, requests, MPI_STATUSES_IGNORE));
        clock_gettime(CLOCK_MONOTONIC_RAW, &t1);

        if (i >= warmup) {
            durations[i - warmup] = getTimeDiffNs(t0, t1);
        }
    }

    uint64_t measurement = 0;
    if (statsType == MpiStatisticsType::Avg) {
        const uint64_t durationAvg = std::accumulate(durations.cbegin(), durations.cend(), static_cast<uint64_t>(0)) / nBatches;
        MPICHK(MPI_Reduce(&durationAvg, &measurement, 1, MPI_UINT64_T, MPI_SUM, 0, MPI_COMM_WORLD));
        measurement /= (2 * batchSize);
    } else if (statsType == MpiStatisticsType::Max) {
        // Need max bandwidth, so find min time
        const uint64_t durationMin = *std::min_element(durations.cbegin(), durations.cend());
        MPICHK(MPI_Reduce(&durationMin, &measurement, 1, MPI_UINT64_T, MPI_MIN, 0, MPI_COMM_WORLD));
    } else if (statsType == MpiStatisticsType::Min) {
        const uint64_t durationMax = *std::max_element(durations.cbegin(), durations.cend());
        MPICHK(MPI_Reduce(&durationMax, &measurement, 1, MPI_UINT64_T, MPI_MAX, 0, MPI_COMM_WORLD));
    }

    if (myRank == 0) {
        std::cout << "**Measurement:" << measurement << std::endl;
    }

    memFree(levelzero, (myRank == 0) ? sendType : recvType, buf);

    MPICHK(MPI_Finalize());

    for (auto i = 0u; i < arguments.iterations; i++) {
        statistics.pushValue(std::chrono::nanoseconds{0}, MeasurementUnit::Unknown, MeasurementType::Unknown);
    }
    return TestResult::Success;
}

TestResult testLatency(const MpiArguments &arguments, Statistics &statistics, const MpiStatisticsType statsType) {
    constexpr uint32_t warmup = 10;
    constexpr uint32_t iterations = 200;

    const uint32_t messageSize = arguments.testArg0;
    const int sendType = arguments.testArg1;
    const int recvType = arguments.testArg2;

    std::vector<uint64_t> durations(iterations);

    LevelZero levelzero{};
    MPICHK(MPI_Init(nullptr, nullptr));

    int myRank;
    MPICHK(MPI_Comm_rank(MPI_COMM_WORLD, &myRank));

    void *sendBuf = memAlloc(levelzero, sendType, messageSize, 1);
    void *recvBuf = memAlloc(levelzero, recvType, messageSize, 1);

    timespec t0, t1;

    for (uint32_t i = 0; i < iterations + warmup; i++) {
        MPICHK(MPI_Barrier(MPI_COMM_WORLD));
        clock_gettime(CLOCK_MONOTONIC_RAW, &t0);
        if (myRank == 0) {
            MPICHK(MPI_Send(sendBuf, messageSize, MPI_INT8_T, 1, 0, MPI_COMM_WORLD));
            MPICHK(MPI_Recv(recvBuf, messageSize, MPI_INT8_T, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE));
        } else {
            MPICHK(MPI_Recv(recvBuf, messageSize, MPI_INT8_T, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE));
            MPICHK(MPI_Send(sendBuf, messageSize, MPI_INT8_T, 0, 0, MPI_COMM_WORLD));
        }
        clock_gettime(CLOCK_MONOTONIC_RAW, &t1);

        if (i >= warmup) {
            durations[i - warmup] = getTimeDiffNs(t0, t1);
        }
    }

    uint64_t measurement = 0;
    if (statsType == MpiStatisticsType::Avg) {
        const uint64_t durationAvg = std::accumulate(durations.cbegin(), durations.cend(), static_cast<uint64_t>(0)) / (2 * iterations);
        MPICHK(MPI_Reduce(&durationAvg, &measurement, 1, MPI_UINT64_T, MPI_SUM, 0, MPI_COMM_WORLD));
        measurement /= 2;
    } else if (statsType == MpiStatisticsType::Max) {
        const uint64_t durationMax = *std::max_element(durations.cbegin(), durations.cend()) / 2;
        MPICHK(MPI_Reduce(&durationMax, &measurement, 1, MPI_UINT64_T, MPI_MAX, 0, MPI_COMM_WORLD));
    } else if (statsType == MpiStatisticsType::Min) {
        const uint64_t durationMin = *std::min_element(durations.cbegin(), durations.cend()) / 2;
        MPICHK(MPI_Reduce(&durationMin, &measurement, 1, MPI_UINT64_T, MPI_MIN, 0, MPI_COMM_WORLD));
    }

    if (myRank == 0) {
        std::cout << "**Measurement:" << measurement << std::endl;
    }

    memFree(levelzero, recvType, recvBuf);
    memFree(levelzero, sendType, sendBuf);

    MPICHK(MPI_Finalize());

    for (auto i = 0u; i < arguments.iterations; i++) {
        statistics.pushValue(std::chrono::nanoseconds{0}, MeasurementUnit::Unknown, MeasurementType::Unknown);
    }
    return TestResult::Success;
}

TestResult testBcast(const MpiArguments &arguments, Statistics &statistics, const MpiStatisticsType statsType) {
    constexpr uint32_t warmup = 5;
    constexpr uint32_t iterations = 50;

    const uint32_t messageSize = arguments.testArg0;
    const int bufferType = arguments.testArg1;

    std::vector<uint64_t> durations(iterations);

    LevelZero levelzero{};
    MPICHK(MPI_Init(nullptr, nullptr));

    int myRank;
    MPICHK(MPI_Comm_rank(MPI_COMM_WORLD, &myRank));

    void *buffer = memAlloc(levelzero, bufferType, messageSize, 1);

    timespec t0, t1;

    for (uint32_t i = 0; i < iterations + warmup; i++) {
        MPICHK(MPI_Barrier(MPI_COMM_WORLD));

        clock_gettime(CLOCK_MONOTONIC_RAW, &t0);
        MPICHK(MPI_Bcast(buffer, messageSize, MPI_INT8_T, 0, MPI_COMM_WORLD));
        clock_gettime(CLOCK_MONOTONIC_RAW, &t1);

        if (i >= warmup) {
            durations[i - warmup] = getTimeDiffNs(t0, t1);
        }
    }

    uint64_t measurement = 0;
    if (statsType == MpiStatisticsType::Avg) {
        const uint64_t durationAvg = std::accumulate(durations.cbegin(), durations.cend(), static_cast<uint64_t>(0)) / iterations;
        MPICHK(MPI_Reduce(&durationAvg, &measurement, 1, MPI_UINT64_T, MPI_SUM, 0, MPI_COMM_WORLD));
        measurement /= 2;
    } else if (statsType == MpiStatisticsType::Max) {
        const uint64_t durationMax = *std::max_element(durations.cbegin(), durations.cend());
        MPICHK(MPI_Reduce(&durationMax, &measurement, 1, MPI_UINT64_T, MPI_MAX, 0, MPI_COMM_WORLD));
    } else if (statsType == MpiStatisticsType::Min) {
        const uint64_t durationMin = *std::min_element(durations.cbegin(), durations.cend());
        MPICHK(MPI_Reduce(&durationMin, &measurement, 1, MPI_UINT64_T, MPI_MIN, 0, MPI_COMM_WORLD));
    }

    if (myRank == 0) {
        std::cout << "**Measurement:" << measurement << std::endl;
    }

    memFree(levelzero, bufferType, buffer);

    MPICHK(MPI_Finalize());

    for (auto i = 0u; i < arguments.iterations; i++) {
        statistics.pushValue(std::chrono::nanoseconds{0}, MeasurementUnit::Unknown, MeasurementType::Unknown);
    }
    return TestResult::Success;
}

TestResult testReduce(const MpiArguments &arguments, Statistics &statistics, const MpiStatisticsType statsType) {
    constexpr uint32_t warmup = 5;
    constexpr uint32_t iterations = 50;

    const uint32_t messageSize = arguments.testArg0;
    const int sendType = arguments.testArg1;
    const int recvType = arguments.testArg2;

    std::vector<uint64_t> durations(iterations);

    LevelZero levelzero{};
    MPICHK(MPI_Init(nullptr, nullptr));

    int myRank;
    MPICHK(MPI_Comm_rank(MPI_COMM_WORLD, &myRank));

    void *sendBuf = memAlloc(levelzero, sendType, messageSize, 1);
    void *recvBuf = memAlloc(levelzero, recvType, messageSize, 1);

    timespec t0, t1;

    for (uint32_t i = 0; i < iterations + warmup; i++) {
        MPICHK(MPI_Barrier(MPI_COMM_WORLD));

        clock_gettime(CLOCK_MONOTONIC_RAW, &t0);
        MPICHK(MPI_Reduce(sendBuf, recvBuf, messageSize, MPI_INT8_T, MPI_SUM, 0, MPI_COMM_WORLD));
        clock_gettime(CLOCK_MONOTONIC_RAW, &t1);

        if (i >= warmup) {
            durations[i - warmup] = getTimeDiffNs(t0, t1);
        }
    }

    uint64_t measurement = 0;
    if (statsType == MpiStatisticsType::Avg) {
        const uint64_t durationAvg = std::accumulate(durations.cbegin(), durations.cend(), static_cast<uint64_t>(0)) / iterations;
        MPICHK(MPI_Reduce(&durationAvg, &measurement, 1, MPI_UINT64_T, MPI_SUM, 0, MPI_COMM_WORLD));
        measurement /= 2;
    } else if (statsType == MpiStatisticsType::Max) {
        const uint64_t durationMax = *std::max_element(durations.cbegin(), durations.cend());
        MPICHK(MPI_Reduce(&durationMax, &measurement, 1, MPI_UINT64_T, MPI_MAX, 0, MPI_COMM_WORLD));
    } else if (statsType == MpiStatisticsType::Min) {
        const uint64_t durationMin = *std::min_element(durations.cbegin(), durations.cend());
        MPICHK(MPI_Reduce(&durationMin, &measurement, 1, MPI_UINT64_T, MPI_MIN, 0, MPI_COMM_WORLD));
    }

    if (myRank == 0) {
        std::cout << "**Measurement:" << measurement << std::endl;
    }

    memFree(levelzero, recvType, recvBuf);
    memFree(levelzero, sendType, sendBuf);

    MPICHK(MPI_Finalize());

    for (auto i = 0u; i < arguments.iterations; i++) {
        statistics.pushValue(std::chrono::nanoseconds{0}, MeasurementUnit::Unknown, MeasurementType::Unknown);
    }
    return TestResult::Success;
}

TestResult testAllReduce(const MpiArguments &arguments, Statistics &statistics, const MpiStatisticsType statsType) {
    constexpr uint32_t warmup = 5;
    constexpr uint32_t iterations = 50;

    const uint32_t messageSize = arguments.testArg0;
    const int sendType = arguments.testArg1;
    const int recvType = arguments.testArg2;

    std::vector<uint64_t> durations(iterations);

    LevelZero levelzero{};
    MPICHK(MPI_Init(nullptr, nullptr));

    int myRank;
    MPICHK(MPI_Comm_rank(MPI_COMM_WORLD, &myRank));

    void *sendBuf = memAlloc(levelzero, sendType, messageSize, 1);
    void *recvBuf = memAlloc(levelzero, recvType, messageSize, 1);

    timespec t0, t1;

    for (uint32_t i = 0; i < iterations + warmup; i++) {
        MPICHK(MPI_Barrier(MPI_COMM_WORLD));

        clock_gettime(CLOCK_MONOTONIC_RAW, &t0);
        MPICHK(MPI_Allreduce(sendBuf, recvBuf, messageSize, MPI_INT8_T, MPI_SUM, MPI_COMM_WORLD));
        clock_gettime(CLOCK_MONOTONIC_RAW, &t1);

        if (i >= warmup) {
            durations[i - warmup] = getTimeDiffNs(t0, t1);
        }
    }

    uint64_t measurement = 0;
    if (statsType == MpiStatisticsType::Avg) {
        const uint64_t durationAvg = std::accumulate(durations.cbegin(), durations.cend(), static_cast<uint64_t>(0)) / iterations;
        MPICHK(MPI_Reduce(&durationAvg, &measurement, 1, MPI_UINT64_T, MPI_SUM, 0, MPI_COMM_WORLD));
        measurement /= 2;
    } else if (statsType == MpiStatisticsType::Max) {
        const uint64_t durationMax = *std::max_element(durations.cbegin(), durations.cend());
        MPICHK(MPI_Reduce(&durationMax, &measurement, 1, MPI_UINT64_T, MPI_MAX, 0, MPI_COMM_WORLD));
    } else if (statsType == MpiStatisticsType::Min) {
        const uint64_t durationMin = *std::min_element(durations.cbegin(), durations.cend());
        MPICHK(MPI_Reduce(&durationMin, &measurement, 1, MPI_UINT64_T, MPI_MIN, 0, MPI_COMM_WORLD));
    }

    if (myRank == 0) {
        std::cout << "**Measurement:" << measurement << std::endl;
    }

    memFree(levelzero, recvType, recvBuf);
    memFree(levelzero, sendType, sendBuf);

    MPICHK(MPI_Finalize());

    for (auto i = 0u; i < arguments.iterations; i++) {
        statistics.pushValue(std::chrono::nanoseconds{0}, MeasurementUnit::Unknown, MeasurementType::Unknown);
    }
    return TestResult::Success;
}

TestResult testAllToAll(const MpiArguments &arguments, Statistics &statistics, const MpiStatisticsType statsType) {
    constexpr uint32_t warmup = 5;
    constexpr uint32_t iterations = 50;

    const uint32_t messageSize = arguments.testArg0;
    const int sendType = arguments.testArg1;
    const int recvType = arguments.testArg2;

    std::vector<uint64_t> durations(iterations);

    LevelZero levelzero{};
    MPICHK(MPI_Init(nullptr, nullptr));

    int myRank, nRanks;
    MPICHK(MPI_Comm_rank(MPI_COMM_WORLD, &myRank));
    MPICHK(MPI_Comm_size(MPI_COMM_WORLD, &nRanks));

    void *sendBuf = memAlloc(levelzero, sendType, messageSize * nRanks, 1);
    void *recvBuf = memAlloc(levelzero, recvType, messageSize * nRanks, 1);

    timespec t0, t1;

    for (uint32_t i = 0; i < iterations + warmup; i++) {
        MPICHK(MPI_Barrier(MPI_COMM_WORLD));

        clock_gettime(CLOCK_MONOTONIC_RAW, &t0);
        MPICHK(MPI_Alltoall(sendBuf, messageSize, MPI_INT8_T, recvBuf, messageSize, MPI_INT8_T, MPI_COMM_WORLD));
        clock_gettime(CLOCK_MONOTONIC_RAW, &t1);

        if (i >= warmup) {
            durations[i - warmup] = getTimeDiffNs(t0, t1);
        }
    }

    uint64_t measurement = 0;
    if (statsType == MpiStatisticsType::Avg) {
        const uint64_t durationAvg = std::accumulate(durations.cbegin(), durations.cend(), static_cast<uint64_t>(0)) / iterations;
        MPICHK(MPI_Reduce(&durationAvg, &measurement, 1, MPI_UINT64_T, MPI_SUM, 0, MPI_COMM_WORLD));
        measurement /= 2;
    } else if (statsType == MpiStatisticsType::Max) {
        const uint64_t durationMax = *std::max_element(durations.cbegin(), durations.cend());
        MPICHK(MPI_Reduce(&durationMax, &measurement, 1, MPI_UINT64_T, MPI_MAX, 0, MPI_COMM_WORLD));
    } else if (statsType == MpiStatisticsType::Min) {
        const uint64_t durationMin = *std::min_element(durations.cbegin(), durations.cend());
        MPICHK(MPI_Reduce(&durationMin, &measurement, 1, MPI_UINT64_T, MPI_MIN, 0, MPI_COMM_WORLD));
    }

    if (myRank == 0) {
        std::cout << "**Measurement:" << measurement << std::endl;
    }

    memFree(levelzero, recvType, recvBuf);
    memFree(levelzero, sendType, sendBuf);

    MPICHK(MPI_Finalize());

    for (auto i = 0u; i < arguments.iterations; i++) {
        statistics.pushValue(std::chrono::nanoseconds{0}, MeasurementUnit::Unknown, MeasurementType::Unknown);
    }
    return TestResult::Success;
}

TestResult run(const MpiArguments &arguments, Statistics &statistics, WorkloadSynchronization &, WorkloadIo &) {
    const auto testType = static_cast<MpiTestType>(static_cast<int>(arguments.testType));
    const auto statsType = static_cast<MpiStatisticsType>(static_cast<int>(arguments.statsType));

    switch (testType) {
    case MpiTestType::Startup: {
        return testStartup(arguments, statistics, statsType);
    }
    case MpiTestType::Bandwidth: {
        return testBandwidth(arguments, statistics, statsType);
    }
    case MpiTestType::Latency: {
        return testLatency(arguments, statistics, statsType);
    }
    case MpiTestType::Bcast: {
        return testBcast(arguments, statistics, statsType);
    }
    case MpiTestType::Reduce: {
        return testReduce(arguments, statistics, statsType);
    }
    case MpiTestType::AllReduce: {
        return testAllReduce(arguments, statistics, statsType);
    }
    case MpiTestType::AllToAll: {
        return testAllToAll(arguments, statistics, statsType);
    }
    default: {
        return TestResult::NoImplementation;
    }
    }
}

int main(int argc, char **argv) {
    MpiWorkload workload;
    MpiWorkload::implementation = run;
    return workload.runFromCommandLine(argc, argv);
}
