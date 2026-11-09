/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/alignment.h"
#include "framework/utility/timer.h"

#include "definitions/usm_copy_reserved_memory.h"

#include <limits>
#include <vector>

using namespace L0;

static TestResult run(const UsmCopyReservedMemoryArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    LevelZero levelzero;

    size_t pageSize = 0;
    ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemQueryPageSize(levelzero.context, levelzero.device, arguments.chunkSize, &pageSize));
    const size_t chunkSize = alignUp(arguments.chunkSize, pageSize);
    const size_t mapCount = arguments.mapCount;
    const size_t reservedSize = chunkSize * mapCount;
    const size_t copySize = arguments.size;
    if (2 * copySize > reservedSize) {
        return TestResult::InvalidArgs;
    }

    void *reservedMemory = nullptr;
    ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemReserve(levelzero.context, nullptr, reservedSize, &reservedMemory));

    ze_physical_mem_desc_t physicalMemoryDesc{ZE_STRUCTURE_TYPE_PHYSICAL_MEM_DESC};
    physicalMemoryDesc.size = chunkSize;
    std::vector<ze_physical_mem_handle_t> physicalMemoryHandles(mapCount, nullptr);
    for (size_t chunk = 0; chunk < mapCount; chunk++) {
        ASSERT_ZE_RESULT_SUCCESS(zePhysicalMemCreate(levelzero.context, levelzero.device, &physicalMemoryDesc, &physicalMemoryHandles[chunk]));
        void *const chunkAddress = static_cast<char *>(reservedMemory) + chunk * chunkSize;
        ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemMap(levelzero.context, chunkAddress, chunkSize, physicalMemoryHandles[chunk], 0, ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE));
    }

    void *const src = reservedMemory;
    void *const dst = static_cast<char *>(reservedMemory) + copySize;

    ze_command_queue_desc_t commandQueueDesc{ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC};
    commandQueueDesc.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
    ze_command_list_handle_t cmdList;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc, &cmdList));

    Timer timer;
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, dst, src, copySize, nullptr, 0, nullptr));
        if (!arguments.measureCompletionTime) {
            timer.measureEnd();
        }
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, std::numeric_limits<uint64_t>::max()));
        if (arguments.measureCompletionTime) {
            timer.measureEnd();
        }
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    for (size_t chunk = 0; chunk < mapCount; chunk++) {
        void *const chunkAddress = static_cast<char *>(reservedMemory) + chunk * chunkSize;
        ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemUnmap(levelzero.context, chunkAddress, chunkSize));
        ASSERT_ZE_RESULT_SUCCESS(zePhysicalMemDestroy(levelzero.context, physicalMemoryHandles[chunk]));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemFree(levelzero.context, reservedMemory, reservedSize));

    return TestResult::Success;
}

static RegisterTestCaseImplementation<UsmCopyReservedMemory> registerTestCase(run, Api::L0);
