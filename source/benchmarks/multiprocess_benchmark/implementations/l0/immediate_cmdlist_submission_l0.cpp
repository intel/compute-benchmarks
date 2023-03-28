/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/process_group.h"

#include "definitions/immediate_cmdlist_submission.h"

#include <gtest/gtest.h>

struct EngineInfo {
    uint32_t ordinal;
    uint32_t engineIndex;
};

static TestResult getComputeEngineInfo(std::vector<EngineInfo> &supportedEngineInfo, LevelZero &levelzero) {
    supportedEngineInfo.clear();

    uint32_t queueGroupPropertiesCount = 0;
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetCommandQueueGroupProperties(levelzero.device, &queueGroupPropertiesCount, nullptr));
    FATAL_ERROR_IF(queueGroupPropertiesCount == 0, "No queue group properties found!");
    std::vector<ze_command_queue_group_properties_t> queueGroupProperties(queueGroupPropertiesCount);
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetCommandQueueGroupProperties(levelzero.device, &queueGroupPropertiesCount, queueGroupProperties.data()));

    auto insertToEngineInfoList = [&supportedEngineInfo](auto &numQueues, auto &ordinal) {
        for (uint32_t j = 0; j < numQueues; j++) {
            supportedEngineInfo.push_back({ordinal, j});
        }
    };

    for (uint32_t i = 0; i < queueGroupPropertiesCount; i++) {
        if ((queueGroupProperties[i].flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) == ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) {
            insertToEngineInfoList(queueGroupProperties[i].numQueues, i);
        }
    }
    return supportedEngineInfo.size() > 0 ? TestResult::Success : TestResult::Error;
}

static TestResult run(const MultiProcessImmediateCmdlistSubmissionArguments &arguments, Statistics &statistics) {
    // Setup
    LevelZero levelzero;
    std::vector<EngineInfo> supportedEngineInfo{};
    const auto status = getComputeEngineInfo(supportedEngineInfo, levelzero);
    if (status != TestResult::Success) {
        return status;
    }

    const auto numberOfSupportedEngines = supportedEngineInfo.size();
    if (supportedEngineInfo.size() * arguments.processesPerEngine > arguments.numberOfProcesses) {
        return TestResult::DeviceNotCapable;
    }

    // Prepare processes
    ProcessGroup processes{"immediate_cmdlist_walker_submission_workload_l0", arguments.numberOfProcesses};
    processes.addArgumentAll("iterations", std::to_string(arguments.iterations));
    processes.addArgumentAll("synchronize", std::string("1"));
    for (auto i = 0u; i < processes.size(); i++) {
        const auto ordinal = supportedEngineInfo[i % numberOfSupportedEngines].ordinal;
        const auto engineIndex = supportedEngineInfo[i % numberOfSupportedEngines].engineIndex;
        processes[i].setName("p:" + std::to_string(i) + "|o:" + std::to_string(ordinal) + "|i:" + std::to_string(engineIndex));
        processes[i].addArgument("ordinal", std::to_string(ordinal));
        processes[i].addArgument("engineIndex", std::to_string(engineIndex));
    }
    // Run processes
    processes.runAll();
    processes.synchronizeAll(arguments.iterations);
    processes.waitForFinishAll();
    TestResult result = processes.getResultAll();
    if (result != TestResult::Success) {
        return result;
    }
    const bool pushIndividualProcessesMeasurements = (processes.size() > 1);
    processes.pushMeasurementsToStatistics(arguments.iterations, statistics, MeasurementUnit::Microseconds,
                                           MeasurementType::Cpu, pushIndividualProcessesMeasurements, true);

    return TestResult::Success;
}

static RegisterTestCaseImplementation<MultiProcessImmediateCmdlistSubmission> registerTestCase(run, Api::L0);
