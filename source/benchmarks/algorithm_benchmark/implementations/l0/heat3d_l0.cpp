/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/process_group.h"

#include "definitions/heat3d.h"

#include <gtest/gtest.h>

#ifdef WIN32

static TestResult run(const Heat3DArguments &, Statistics &) {
    return TestResult::NoImplementation;
}

#else // WIN32

#include "framework/utility/linux/ipc.h"

#ifndef USE_PIDFD
const std::string masterSocketName{"/tmp/heat3d.socket"};
#endif // USE_PIDFD

static TestResult ipcBarrierMaster(std::vector<ze_event_handle_t> &barrierEvents) {
    const uint32_t nRanks = barrierEvents.size() / 2;

    // Wait for worker processes
    for (uint32_t i = 0; i < nRanks; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(barrierEvents[i], UINT64_MAX));
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(barrierEvents[i]));
    }

    // Release worker processes
    for (uint32_t i = 0; i < nRanks; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSignal(barrierEvents[i + nRanks]));
    }

    return TestResult::Success;
}

static TestResult run(const Heat3DArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    const uint32_t nRanks = arguments.subDomainX * arguments.subDomainY * arguments.subDomainZ;

    LevelZero levelzero{};

    uint32_t tilesCount = 0;
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetSubDevices(levelzero.device, &tilesCount, nullptr));
    if (tilesCount > 1) {
        return TestResult::DeviceNotCapable;
    }

    if ((levelzero.getIpcProperties().flags & ZE_IPC_PROPERTY_FLAG_MEMORY) == 0) {
        return TestResult::DeviceNotCapable;
    }

    ze_host_mem_alloc_desc_t initBufferDesc = {};
    initBufferDesc.stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC;
    initBufferDesc.pNext = nullptr;
    initBufferDesc.flags = ZE_HOST_MEM_ALLOC_FLAG_BIAS_UNCACHED;
    void *initBuffer;
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocHost(levelzero.context, &initBufferDesc, 6 * nRanks * 1024, 0, &initBuffer));
    std::fill_n(static_cast<uint8_t *>(initBuffer), 6 * nRanks * 1024, 0);
    ze_ipc_mem_handle_t initBufferIpcHandle;
    std::fill_n(initBufferIpcHandle.data, ZE_MAX_IPC_HANDLE_SIZE, static_cast<char>(0));
    ASSERT_ZE_RESULT_SUCCESS(zeMemGetIpcHandle(levelzero.context, initBuffer, &initBufferIpcHandle));

    ze_event_pool_desc_t barrierEvPoolDesc = {};
    barrierEvPoolDesc.stype = ZE_STRUCTURE_TYPE_EVENT_POOL_DESC;
    barrierEvPoolDesc.pNext = nullptr;
    barrierEvPoolDesc.flags = ZE_EVENT_POOL_FLAG_IPC | ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
    barrierEvPoolDesc.count = 2 * nRanks;
    ze_event_pool_handle_t barrierEvPool;
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &barrierEvPoolDesc, 0, nullptr, &barrierEvPool));
    ze_ipc_event_pool_handle_t barrierEvPoolIpcHandle;
    std::fill_n(barrierEvPoolIpcHandle.data, ZE_MAX_IPC_HANDLE_SIZE, static_cast<char>(0));
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolGetIpcHandle(barrierEvPool, &barrierEvPoolIpcHandle));

    std::vector<ze_event_handle_t> barrierEvents(2 * nRanks);
    ze_event_desc_t barrierEventDesc = {};
    barrierEventDesc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
    barrierEventDesc.pNext = nullptr;
    barrierEventDesc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
    barrierEventDesc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
    for (uint32_t i = 0; i < 2 * nRanks; i++) {
        barrierEventDesc.index = i;
        ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(barrierEvPool, &barrierEventDesc, &barrierEvents[i]));
    }

    ProcessGroup processes{"heat3d_workload_l0", nRanks};
    processes.addArgumentAll("iterations", std::to_string(arguments.iterations));
    processes.addArgumentAll("parentPid", std::to_string(getpid()));
#ifdef USE_PIDFD
    processes.addArgumentAll("initBufferIpcHandle", serializeBinaryToStr(initBufferIpcHandle.data, ZE_MAX_IPC_HANDLE_SIZE));
    processes.addArgumentAll("evPoolIpcHandle", serializeBinaryToStr(barrierEvPoolIpcHandle.data, ZE_MAX_IPC_HANDLE_SIZE));
#endif // USE_PIDFD
    processes.addArgumentAll("nSubDomainX", std::to_string(arguments.subDomainX));
    processes.addArgumentAll("nSubDomainY", std::to_string(arguments.subDomainY));
    processes.addArgumentAll("nSubDomainZ", std::to_string(arguments.subDomainZ));
    processes.addArgumentAll("nTimesteps", std::to_string(arguments.timesteps));
    processes.addArgumentAll("meshLength", std::to_string(arguments.meshLength));

    for (auto i = 0u; i < processes.size(); i++) {
        processes[i].addArgument("rank", std::to_string(i));
        processes[i].setName("L0 Heat3D Process #" + std::to_string(i));
    }

    processes.runAll();

    // For initParams
#ifdef USE_PIDFD
    // Wait for IPC handles to be written into the initBuffer
    ipcBarrierMaster(barrierEvents);
#else
    int socketMaster = -1;
    socketCreate(socketMaster);
    socketBindAndListen(socketMaster, masterSocketName);

    std::vector<int> socketWorkers{};
    for (uint32_t i = 0; i < nRanks; i++) {
        int socketWorker = -1;
        socketAccept(socketMaster, socketWorkers, socketWorker);
        socketSendDataWithFd(socketWorker, *reinterpret_cast<int *>(initBufferIpcHandle.data), initBufferIpcHandle.data, ZE_MAX_IPC_HANDLE_SIZE);
        socketSendDataWithFd(socketWorker, *reinterpret_cast<int *>(barrierEvPoolIpcHandle.data), barrierEvPoolIpcHandle.data, ZE_MAX_IPC_HANDLE_SIZE);
    }

    for (auto sd : socketWorkers) {
        EXPECT_EQ(0, close(sd));
    }
    EXPECT_EQ(0, close(socketMaster));

    // Synchronize all the ranks when they exchange IPC handles through socket
    for (uint32_t r = 0; r < nRanks; r++) {
        if (nRanks == 1) {
            break;
        }
        ipcBarrierMaster(barrierEvents);
        ipcBarrierMaster(barrierEvents);
    }
#endif // USE_PIDFD

    for (auto i = 0u; i < arguments.iterations; i++) {
        // Init unpacking the receive buffers
        ipcBarrierMaster(barrierEvents);

        processes.synchronizeAll(1);

        for (size_t t = 0; t < arguments.timesteps; t++) {
            ipcBarrierMaster(barrierEvents);
        }
    }

    processes.waitForFinishAll();

    TestResult result = processes.getResultAll();
    if (result != TestResult::Success) {
        return result;
    }

    const bool pushIndividualProcessesMeasurements = (processes.size() > 1);
    processes.pushMeasurementsToStatistics(arguments.iterations, statistics, typeSelector.getUnit(),
                                           typeSelector.getType(), pushIndividualProcessesMeasurements, true);

#ifndef USE_PIDFD
    EXPECT_EQ(0, unlink(masterSocketName.c_str()));
#endif // USE_PIDFD

    for (auto ev : barrierEvents) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(ev));
    }
    // TODO: this fails
    zeEventPoolPutIpcHandle(levelzero.context, barrierEvPoolIpcHandle);
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(barrierEvPool));
    ASSERT_ZE_RESULT_SUCCESS(zeMemPutIpcHandle(levelzero.context, initBufferIpcHandle));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, initBuffer));

    return TestResult::Success;
}

#endif // WIN32

static RegisterTestCaseImplementation<Heat3D> registerTestCase(run, Api::L0);
