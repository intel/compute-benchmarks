/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/append_metric_streamer_marker.h"
#include "utility/l0/metric_group_utility_l0.h"

#include <gtest/gtest.h>

using namespace L0;

static TestResult run(const AppendMetricStreamerMarkerArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    LevelZero levelzero;
    Timer timer;

    if (isPerformanceCounterStreamRestricted()) {
        return TestResult::DeviceNotCapable;
    }

    constexpr uint32_t samplingPeriodNs = 1'000'000u;
    constexpr uint32_t notifyEveryNReports = 32'768u;
    zet_metric_group_handle_t metricGroup{};
    zet_metric_streamer_handle_t streamer{};
    for (zet_metric_group_handle_t candidateGroup : getMetricGroups(levelzero.device, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED)) {
        ASSERT_ZE_RESULT_SUCCESS(zetContextActivateMetricGroups(levelzero.context, levelzero.device, 1u, &candidateGroup));

        zet_metric_streamer_desc_t streamerDesc{ZET_STRUCTURE_TYPE_METRIC_STREAMER_DESC};
        streamerDesc.notifyEveryNReports = notifyEveryNReports;
        streamerDesc.samplingPeriod = samplingPeriodNs;
        ze_result_t result = zetMetricStreamerOpen(levelzero.context, levelzero.device, candidateGroup, &streamerDesc, nullptr, &streamer);
        if (result == ZE_RESULT_SUCCESS) {
            result = probeAppend(levelzero, [streamer](ze_command_list_handle_t cmdList) { return zetCommandListAppendMetricStreamerMarker(cmdList, streamer, 0u); });
            if (result == ZE_RESULT_SUCCESS) {
                metricGroup = candidateGroup;
                break;
            }
            ASSERT_ZE_RESULT_SUCCESS(zetMetricStreamerClose(streamer));
        }

        ASSERT_ZE_RESULT_SUCCESS(zetContextActivateMetricGroups(levelzero.context, levelzero.device, 0u, nullptr));
        if (!isMetricCollectionUnavailable(result)) {
            ASSERT_ZE_RESULT_SUCCESS(result);
        }
    }
    if (metricGroup == nullptr) {
        return TestResult::DeviceNotCapable;
    }

    ze_command_list_desc_t cmdListDesc{ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;

    for (auto i = 0u; i < arguments.iterations; ++i) {
        ze_command_list_handle_t cmdList;
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));

        timer.measureStart();
        for (auto j = 0u; j < arguments.appendCount; ++j) {
            ASSERT_ZE_RESULT_SUCCESS(zetCommandListAppendMetricStreamerMarker(cmdList, streamer, j));
        }
        timer.measureEnd();
        statistics.pushValue(timer.get() / arguments.appendCount, typeSelector.getUnit(), typeSelector.getType());

        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    }

    ASSERT_ZE_RESULT_SUCCESS(zetMetricStreamerClose(streamer));
    ASSERT_ZE_RESULT_SUCCESS(zetContextActivateMetricGroups(levelzero.context, levelzero.device, 0u, nullptr));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<AppendMetricStreamerMarker> registerTestCase(run, Api::L0);
