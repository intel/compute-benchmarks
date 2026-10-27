/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/l0/levelzero.h"

#include <level_zero/zet_api.h>
#include <vector>

#if defined(__linux__)
#include <fstream>
#include <unistd.h>
#endif

template <typename IsMatchingGroup>
std::vector<zet_metric_group_handle_t> getMetricGroupsIf(ze_device_handle_t device, const IsMatchingGroup &isMatchingGroup) {
    uint32_t metricGroupCount = 0u;
    if (zetMetricGroupGet(device, &metricGroupCount, nullptr) != ZE_RESULT_SUCCESS || metricGroupCount == 0u) {
        return {};
    }
    std::vector<zet_metric_group_handle_t> metricGroups(metricGroupCount);
    if (zetMetricGroupGet(device, &metricGroupCount, metricGroups.data()) != ZE_RESULT_SUCCESS) {
        return {};
    }

    std::vector<zet_metric_group_handle_t> matchingGroups;
    for (const zet_metric_group_handle_t metricGroup : metricGroups) {
        if (isMatchingGroup(metricGroup)) {
            matchingGroups.push_back(metricGroup);
        }
    }
    return matchingGroups;
}

inline std::vector<zet_metric_group_handle_t> getMetricGroups(ze_device_handle_t device, zet_metric_group_sampling_type_flag_t samplingType) {
    return getMetricGroupsIf(device, [samplingType](zet_metric_group_handle_t metricGroup) {
        zet_metric_group_properties_t properties{ZET_STRUCTURE_TYPE_METRIC_GROUP_PROPERTIES};
        return zetMetricGroupGetProperties(metricGroup, &properties) == ZE_RESULT_SUCCESS && (properties.samplingType & samplingType) != 0;
    });
}

inline std::vector<zet_metric_group_handle_t> getMarkerMetricGroups(ze_device_handle_t device) {
    return getMetricGroupsIf(device, [](zet_metric_group_handle_t metricGroup) {
        zet_metric_group_type_exp_t groupType{ZET_STRUCTURE_TYPE_METRIC_GROUP_TYPE_EXP};
        zet_metric_group_properties_t properties{ZET_STRUCTURE_TYPE_METRIC_GROUP_PROPERTIES};
        properties.pNext = &groupType;
        return zetMetricGroupGetProperties(metricGroup, &properties) == ZE_RESULT_SUCCESS && (groupType.type & ZET_METRIC_GROUP_TYPE_EXP_FLAG_MARKER) != 0;
    });
}

inline zet_metric_group_handle_t findMetricGroup(ze_device_handle_t device, zet_metric_group_sampling_type_flag_t samplingType) {
    const std::vector<zet_metric_group_handle_t> metricGroups = getMetricGroups(device, samplingType);
    return metricGroups.empty() ? nullptr : metricGroups.front();
}

// Without access to the performance counter stream, the xe driver fails zetMetricStreamerOpen with
// ZE_RESULT_ERROR_UNKNOWN and zetMetricQueryPoolCreate with ZE_RESULT_ERROR_INVALID_ARGUMENT,
// not with ZE_RESULT_ERROR_INSUFFICIENT_PERMISSIONS, so the restriction is read from the kernel setting.
inline bool isPerformanceCounterStreamRestricted() {
#if defined(__linux__)
    if (geteuid() == 0) {
        return false;
    }
    for (const char *paranoidSettingPath : {"/proc/sys/dev/xe/observation_paranoid", "/proc/sys/dev/i915/perf_stream_paranoid"}) {
        std::ifstream paranoidSetting(paranoidSettingPath);
        int paranoidLevel = 0;
        if (paranoidSetting >> paranoidLevel) {
            return paranoidLevel != 0;
        }
    }
#endif
    return false;
}

inline bool isMetricCollectionUnavailable(ze_result_t result) {
    return result == ZE_RESULT_ERROR_INSUFFICIENT_PERMISSIONS ||
           result == ZE_RESULT_ERROR_NOT_AVAILABLE ||
           result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE;
}

template <typename Append>
ze_result_t probeAppend(const L0::LevelZero &levelzero, const Append &append) {
    ze_command_list_desc_t cmdListDesc{ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ze_command_list_handle_t cmdList{};
    if (const ze_result_t result = zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList); result != ZE_RESULT_SUCCESS) {
        return result;
    }
    const ze_result_t appendResult = append(cmdList);
    const ze_result_t destroyResult = zeCommandListDestroy(cmdList);
    return appendResult != ZE_RESULT_SUCCESS ? appendResult : destroyResult;
}
