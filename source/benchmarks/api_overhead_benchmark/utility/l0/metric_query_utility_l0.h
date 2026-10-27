/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/configuration.h"
#include "framework/l0/levelzero.h"
#include "framework/test_case/test_result.h"
#include "framework/utility/statistics.h"
#include "framework/utility/timer.h"

#include "definitions/append_metric_query_arguments.h"
#include "utility/l0/metric_group_utility_l0.h"

#include <level_zero/zet_api.h>
#include <vector>

enum class MetricQueryAppend {
    Begin,
    End,
};

inline TestResult measureMetricQueryAppend(const AppendMetricQueryArguments &arguments, Statistics &statistics,
                                           MetricQueryAppend measuredAppend, bool useSignalEvent) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    L0::LevelZero levelzero;
    Timer timer;

    if (isPerformanceCounterStreamRestricted()) {
        return TestResult::DeviceNotCapable;
    }

    zet_metric_group_handle_t metricGroup = findMetricGroup(levelzero.device, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_EVENT_BASED);
    if (metricGroup == nullptr) {
        return TestResult::DeviceNotCapable;
    }
    ASSERT_ZE_RESULT_SUCCESS(zetContextActivateMetricGroups(levelzero.context, levelzero.device, 1u, &metricGroup));

    const uint32_t queryCount = static_cast<uint32_t>(arguments.appendCount);
    zet_metric_query_pool_desc_t queryPoolDesc{ZET_STRUCTURE_TYPE_METRIC_QUERY_POOL_DESC};
    queryPoolDesc.type = ZET_METRIC_QUERY_POOL_TYPE_PERFORMANCE;
    queryPoolDesc.count = queryCount;
    zet_metric_query_pool_handle_t queryPool{};
    if (const ze_result_t result = zetMetricQueryPoolCreate(levelzero.context, levelzero.device, metricGroup, &queryPoolDesc, &queryPool);
        result != ZE_RESULT_SUCCESS) {
        ASSERT_ZE_RESULT_SUCCESS(zetContextActivateMetricGroups(levelzero.context, levelzero.device, 0u, nullptr));
        if (isMetricCollectionUnavailable(result)) {
            return TestResult::DeviceNotCapable;
        }
        ASSERT_ZE_RESULT_SUCCESS(result);
    }

    std::vector<zet_metric_query_handle_t> queries(queryCount);
    for (auto j = 0u; j < queryCount; ++j) {
        ASSERT_ZE_RESULT_SUCCESS(zetMetricQueryCreate(queryPool, j, &queries[j]));
    }

    ze_event_pool_handle_t eventPool{};
    std::vector<ze_event_handle_t> events(queryCount);
    if (useSignalEvent) {
        const ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr, ZE_EVENT_POOL_FLAG_HOST_VISIBLE, queryCount};
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 0, nullptr, &eventPool));
        ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, 0, ZE_EVENT_SCOPE_FLAG_HOST, ZE_EVENT_SCOPE_FLAG_HOST};
        for (auto j = 0u; j < queryCount; ++j) {
            eventDesc.index = j;
            ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &events[j]));
        }
    }

    ze_command_list_desc_t cmdListDesc{ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;

    for (auto i = 0u; i < arguments.iterations; ++i) {
        for (const zet_metric_query_handle_t query : queries) {
            ASSERT_ZE_RESULT_SUCCESS(zetMetricQueryReset(query));
        }
        ze_command_list_handle_t cmdList;
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));

        if (measuredAppend == MetricQueryAppend::Begin) {
            timer.measureStart();
        }
        for (auto j = 0u; j < queryCount; ++j) {
            ASSERT_ZE_RESULT_SUCCESS(zetCommandListAppendMetricQueryBegin(cmdList, queries[j]));
        }
        if (measuredAppend == MetricQueryAppend::Begin) {
            timer.measureEnd();
        } else {
            timer.measureStart();
        }
        for (auto j = 0u; j < queryCount; ++j) {
            ASSERT_ZE_RESULT_SUCCESS(zetCommandListAppendMetricQueryEnd(cmdList, queries[j], events[j], 0u, nullptr));
        }
        if (measuredAppend == MetricQueryAppend::End) {
            timer.measureEnd();
        }
        statistics.pushValue(timer.get() / queryCount, typeSelector.getUnit(), typeSelector.getType());

        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    }

    for (const ze_event_handle_t event : events) {
        if (event != nullptr) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
        }
    }
    if (eventPool != nullptr) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    }
    for (const zet_metric_query_handle_t query : queries) {
        ASSERT_ZE_RESULT_SUCCESS(zetMetricQueryDestroy(query));
    }
    ASSERT_ZE_RESULT_SUCCESS(zetMetricQueryPoolDestroy(queryPool));
    ASSERT_ZE_RESULT_SUCCESS(zetContextActivateMetricGroups(levelzero.context, levelzero.device, 0u, nullptr));
    return TestResult::Success;
}
