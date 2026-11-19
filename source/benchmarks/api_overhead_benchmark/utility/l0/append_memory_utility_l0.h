/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/enum/usm_memory_placement.h"
#include "framework/l0/levelzero.h"
#include "framework/l0/utility/usable_memory_helper.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/timer.h"

#include <cstddef>
#include <initializer_list>
#include <limits>
#include <utility>
#include <vector>

namespace L0 {

class BigUsmAllocations {
  public:
    explicit BigUsmAllocations(LevelZero &levelzero) : levelzero(levelzero) {}
    BigUsmAllocations(const BigUsmAllocations &) = delete;
    BigUsmAllocations &operator=(const BigUsmAllocations &) = delete;
    ~BigUsmAllocations() {
        for (const auto &[placement, buffer] : this->allocations) {
            EXPECT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(placement, this->levelzero, buffer));
        }
    }

    static bool fitsInDeviceMemory(LevelZero &levelzero, std::initializer_list<std::pair<UsmMemoryPlacement, size_t>> requests) {
        uint64_t required = 0;
        for (const auto &[placement, size] : requests) {
            if (isDeviceMemoryType(placement)) {
                required += size;
            }
        }
        return required <= UsableMemoryHelper::queryOrTotal(levelzero.device).value_or(0);
    }

    TestResult allocate(UsmMemoryPlacement placement, size_t size, void **buffer) {
        const ze_result_t result = UsmHelper::allocateWithRelaxedLimits(placement, this->levelzero, size, buffer);
        if (result == ZE_RESULT_ERROR_OUT_OF_DEVICE_MEMORY || result == ZE_RESULT_ERROR_OUT_OF_HOST_MEMORY || result == ZE_RESULT_ERROR_UNSUPPORTED_SIZE) {
            return TestResult::DeviceNotCapable;
        }
        ASSERT_ZE_RESULT_SUCCESS(result);
        this->allocations.emplace_back(placement, *buffer);
        return TestResult::Success;
    }

  private:
    LevelZero &levelzero;
    std::vector<std::pair<UsmMemoryPlacement, void *>> allocations;
};

template <typename AppendMemory>
TestResult measureAppendMemory(LevelZero &levelzero, const TestCaseArgumentContainer &arguments, bool useEvent, uint32_t appendCount, bool useImmediate,
                               Statistics &statistics, const MeasurementFields &typeSelector, AppendMemory &&appendMemory) {
    Timer timer;

    const ze_event_pool_desc_t eventPoolDesc = {ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr, 0, appendCount};
    ze_event_desc_t eventDesc = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, 0, ZE_EVENT_SCOPE_FLAG_DEVICE, ZE_EVENT_SCOPE_FLAG_DEVICE};
    ze_event_pool_handle_t eventPool{};
    std::vector<ze_event_handle_t> events(appendCount);
    if (useEvent) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 0, nullptr, &eventPool));
        for (auto j = 0u; j < appendCount; ++j) {
            eventDesc.index = j;
            ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &events[j]));
        }
    }

    ze_command_list_handle_t cmdList{};
    if (useImmediate) {
        ze_command_queue_desc_t commandQueueDesc = levelzero.commandQueueDesc;
        commandQueueDesc.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc, &cmdList));
    } else {
        ze_command_list_desc_t cmdListDesc{ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC};
        cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));
    }

    for (auto i = 0u; i < arguments.iterations; ++i) {
        timer.measureStart();
        for (auto j = 0u; j < appendCount; ++j) {
            ASSERT_ZE_RESULT_SUCCESS(appendMemory(cmdList, events[j]));
        }
        timer.measureEnd();
        if (useImmediate) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, std::numeric_limits<uint64_t>::max()));
            if (useEvent) {
                for (auto j = 0u; j < appendCount; ++j) {
                    ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(events[j]));
                }
            }
        } else {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListReset(cmdList));
        }
        statistics.pushValue(timer.get() / appendCount, typeSelector.getUnit(), typeSelector.getType());
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    if (useEvent) {
        for (auto j = 0u; j < appendCount; ++j) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(events[j]));
        }
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    }

    return TestResult::Success;
}

} // namespace L0
