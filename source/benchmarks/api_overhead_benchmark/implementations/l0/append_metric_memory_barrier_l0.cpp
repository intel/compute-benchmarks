/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/append_metric_memory_barrier.h"
#include "utility/l0/metric_group_utility_l0.h"

#include <gtest/gtest.h>

using namespace L0;

static TestResult run(const AppendMetricMemoryBarrierArguments &arguments, Statistics &statistics) {
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

    zet_metric_group_handle_t metricGroup = findMetricGroup(levelzero.device, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_EVENT_BASED);
    if (metricGroup == nullptr) {
        return TestResult::DeviceNotCapable;
    }
    ASSERT_ZE_RESULT_SUCCESS(zetContextActivateMetricGroups(levelzero.context, levelzero.device, 1u, &metricGroup));

    zet_metric_query_pool_desc_t queryPoolDesc{ZET_STRUCTURE_TYPE_METRIC_QUERY_POOL_DESC};
    queryPoolDesc.type = ZET_METRIC_QUERY_POOL_TYPE_PERFORMANCE;
    queryPoolDesc.count = 1u;
    zet_metric_query_pool_handle_t queryPool{};
    if (const ze_result_t result = zetMetricQueryPoolCreate(levelzero.context, levelzero.device, metricGroup, &queryPoolDesc, &queryPool);
        result != ZE_RESULT_SUCCESS) {
        ASSERT_ZE_RESULT_SUCCESS(zetContextActivateMetricGroups(levelzero.context, levelzero.device, 0u, nullptr));
        if (isMetricCollectionUnavailable(result)) {
            return TestResult::DeviceNotCapable;
        }
        ASSERT_ZE_RESULT_SUCCESS(result);
    }
    zet_metric_query_handle_t query{};
    ASSERT_ZE_RESULT_SUCCESS(zetMetricQueryCreate(queryPool, 0u, &query));

    ze_command_list_desc_t cmdListDesc{ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;

    for (auto i = 0u; i < arguments.iterations; ++i) {
        ASSERT_ZE_RESULT_SUCCESS(zetMetricQueryReset(query));
        ze_command_list_handle_t cmdList;
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));
        ASSERT_ZE_RESULT_SUCCESS(zetCommandListAppendMetricQueryBegin(cmdList, query));

        timer.measureStart();
        for (auto j = 0u; j < arguments.appendCount; ++j) {
            ASSERT_ZE_RESULT_SUCCESS(zetCommandListAppendMetricMemoryBarrier(cmdList));
        }
        timer.measureEnd();
        statistics.pushValue(timer.get() / arguments.appendCount, typeSelector.getUnit(), typeSelector.getType());

        ASSERT_ZE_RESULT_SUCCESS(zetCommandListAppendMetricQueryEnd(cmdList, query, nullptr, 0u, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    }

    ASSERT_ZE_RESULT_SUCCESS(zetMetricQueryDestroy(query));
    ASSERT_ZE_RESULT_SUCCESS(zetMetricQueryPoolDestroy(queryPool));
    ASSERT_ZE_RESULT_SUCCESS(zetContextActivateMetricGroups(levelzero.context, levelzero.device, 0u, nullptr));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<AppendMetricMemoryBarrier> registerTestCase(run, Api::L0);
