/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/process_group.h"

#include "definitions/multi_process_compute_shared_buffer.h"
#include "utility/l0/multi_process_helper_l0.h"

#include <gtest/gtest.h>

static TestResult run(const MultiProcessComputeSharedBufferArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    ContextProperties contexProperties = ContextProperties::create().setDeviceSelection(arguments.deviceSelection).createSingleFakeSubDeviceIfNeeded();
    QueueProperties queueProperties = QueueProperties::create().disable();
    LevelZero levelzero{queueProperties, contexProperties};

    // We need IPC to run this benchmark
    if ((levelzero.getIpcProperties().flags & ZE_IPC_PROPERTY_FLAG_MEMORY) == 0) {
        return TestResult::DeviceNotCapable;
    }

    // Get tiles for execution, validate if they are available
    std::vector<DeviceSelection> subDevicesForExecution = {};
    ASSERT_ZE_RESULT_SUCCESS(MultiProcessHelperL0::getSubDevicesForExecution(levelzero, arguments.deviceSelection, arguments.processesPerTile, subDevicesForExecution));
    if (subDevicesForExecution.size() == 0) {
        return TestResult::DeviceNotCapable;
    }

    // Create a buffer for each tile
    std::unordered_map<DeviceSelection, MultiProcessHelperL0::BufferForSubDevice> buffersForSubDevices = {};
    ASSERT_ZE_RESULT_SUCCESS(MultiProcessHelperL0::allocateSharedBuffersForSubDevices(levelzero, arguments.deviceSelection,
                                                                                      arguments.processesPerTile,
                                                                                      arguments.workgroupsPerProcess,
                                                                                      MultiProcessHelperL0::workloadWorkgroupSize,
                                                                                      buffersForSubDevices));

    // Prepare processes
    ProcessGroup processes{"single_queue_workload_shared_buffer_l0", subDevicesForExecution.size()};
    processes.addArgumentAll("iterations", std::to_string(arguments.iterations));
    processes.addArgumentAll("synchronize", std::to_string(arguments.synchronize));
    processes.addArgumentAll("operationsCount", std::to_string(MultiProcessHelperL0::workloadOperationsCount));
    processes.addArgumentAll("wgc", std::to_string(arguments.workgroupsPerProcess));
    processes.addArgumentAll("wgs", std::to_string(MultiProcessHelperL0::workloadWorkgroupSize));
    for (auto i = 0u; i < processes.size(); i++) {
        MultiProcessHelperL0::BufferForSubDevice &buffer = buffersForSubDevices[subDevicesForExecution[i]];
        processes[i].addHandleForInheritance(buffer.fileDescriptor);
        processes[i].addArgument("bufferIpcHandle", std::to_string(buffer.fileDescriptor));
        processes[i].addArgument("bufferOffset", std::to_string(buffer.getNextOffset()));
        processes[i].addEnvVariable("ZE_AFFINITY_MASK", MultiProcessHelperL0::createAffinityMask(levelzero.rootDeviceIndex, subDevicesForExecution[i]));
        processes[i].setName(MultiProcessHelperL0::createProcessName(subDevicesForExecution, i));
    }

    // Run processes
    processes.runAll();
    if (arguments.synchronize) {
        processes.synchronizeAll(arguments.iterations);
    }
    processes.waitForFinishAll();
    if (TestResult result = processes.getResultAll(); result != TestResult::Success) {
        return result;
    }
    const bool pushIndividualProcessesMeasurements = (processes.size() > 1);
    processes.pushMeasurementsToStatistics(arguments.iterations, statistics, typeSelector.getUnit(),
                                           typeSelector.getType(), pushIndividualProcessesMeasurements, true);

    // Free allocated buffers
    for (const auto &bufferForSubDevice : buffersForSubDevices) {
        void *buffer = bufferForSubDevice.second.buffer;
        ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, buffer));
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<MultiProcessComputeSharedBuffer> registerTestCase(run, Api::L0);
