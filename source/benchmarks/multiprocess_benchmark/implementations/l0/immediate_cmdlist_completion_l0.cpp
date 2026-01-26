/*
 * Copyright (C) 2023-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/process_group.h"

#include "definitions/immediate_cmdlist_completion.h"

#include <gtest/gtest.h>

struct EngineInfo {
    uint32_t ordinal;
    uint32_t engineIndex;
};

static TestResult getEngineInfo(std::vector<EngineInfo> &supportedEngineInfo, LevelZero &levelzero, std::string_view engineGroup, const MultiProcessImmediateCmdlistCompletionArguments &arguments) {
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
        if (engineGroup.compare("Compute") == 0) {
            if ((queueGroupProperties[i].flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) == ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) {
                insertToEngineInfoList(queueGroupProperties[i].numQueues, i);
            }
        } else {
            // Use all Copy Engines (main and link)
            if (queueGroupProperties[i].flags == ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY) {
                insertToEngineInfoList(queueGroupProperties[i].numQueues, i);
            }
        }
    }
    // Apply the mask
    std::vector<EngineInfo>::iterator it = supportedEngineInfo.begin();
    std::bitset<maxNumberOfEngines> engineBitset = arguments.engineMask;
    uint32_t engineMaskPosition = 0;
    while (it != supportedEngineInfo.end()) {
        if (engineBitset.test(engineMaskPosition) == false) {
            it = supportedEngineInfo.erase(it);
        } else {
            ++it;
        }
        engineMaskPosition++;
    }
    return supportedEngineInfo.size() > 0 ? TestResult::Success : TestResult::Error;
}

static TestResult run(const MultiProcessImmediateCmdlistCompletionArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    LevelZero levelzero(QueueProperties::create().disable());

    std::vector<EngineInfo> supportedEngineInfo{};
    std::string engineGroup = static_cast<const std::string &>(arguments.engineGroup);
    const auto status = getEngineInfo(supportedEngineInfo, levelzero, engineGroup, arguments);
    if (status != TestResult::Success) {
        return status;
    }

    const auto numberOfSupportedEngines = supportedEngineInfo.size();

    // Prepare processes
    ProcessGroup processes{"immediate_cmdlist_copy_workload_l0", arguments.numberOfProcesses};
    processes.addArgumentAll("iterations", std::to_string(arguments.iterations));
    processes.addArgumentAll("synchronize", std::string("1"));
    for (auto i = 0u; i < processes.size(); i++) {
        const auto ordinal = supportedEngineInfo[i % numberOfSupportedEngines].ordinal;
        const auto engineIndex = supportedEngineInfo[i % numberOfSupportedEngines].engineIndex;
        processes[i].setName("p:" + std::to_string(i) + "|o:" + std::to_string(ordinal) + "|i:" + std::to_string(engineIndex));
        processes[i].addArgument("ordinal", std::to_string(ordinal));
        processes[i].addArgument("engineIndex", std::to_string(engineIndex));
        processes[i].addArgument("withCopyOffload", "1");
    }
    processes.addArgumentAll("copySize", std::to_string(arguments.copySize));
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

static RegisterTestCaseImplementation<MultiProcessImmediateCmdlistCompletion> registerTestCase(run, Api::L0);
