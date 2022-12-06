/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/process_group.h"

#include "definitions/multi_process_compute.h"
#include "utility/l0/multi_process_helper_l0.h"

#include <gtest/gtest.h>

static TestResult run(const MultiProcessComputeArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    ContextProperties contexProperties = ContextProperties::create().setDeviceSelection(arguments.deviceSelection).createSingleFakeSubDeviceIfNeeded();
    QueueProperties queueProperties = QueueProperties::create().disable();
    LevelZero levelzero{queueProperties, contexProperties};

    // Get tiles for execution, validate if they are available
    std::vector<DeviceSelection> subDevicesForExecution = {};
    ASSERT_ZE_RESULT_SUCCESS(MultiProcessHelperL0::getSubDevicesForExecution(levelzero, arguments.deviceSelection, arguments.processesPerTile, subDevicesForExecution));
    if (subDevicesForExecution.size() == 0) {
        return TestResult::DeviceNotCapable;
    }

    // Prepare processes
    ProcessGroup processes{"single_queue_workload_l0", subDevicesForExecution.size()};
    processes.addArgumentAll("iterations", std::to_string(arguments.iterations));
    processes.addArgumentAll("synchronize", std::to_string(arguments.synchronize));
    processes.addArgumentAll("operationsCount", std::to_string(arguments.operationsPerKernelCount));
    processes.addArgumentAll("wgc", std::to_string(arguments.workgroupsPerProcess));
    processes.addArgumentAll("wgs", std::to_string(MultiProcessHelperL0::workloadWorkgroupSize));
    for (auto i = 0u; i < processes.size(); i++) {
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

    return TestResult::Success;
}

static RegisterTestCaseImplementation<MultiProcessCompute> registerTestCase(run, Api::L0);
