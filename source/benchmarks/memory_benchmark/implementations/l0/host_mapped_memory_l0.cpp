/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/host_mapped_memory.h"

#include <atomic>
#include <cstring>
#include <gtest/gtest.h>

static TestResult run(const HostMappedMemoryArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::GigabytesPerSecond, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    switch (arguments.type) {
    case StreamMemoryType::Read:
    case StreamMemoryType::Write:
        break;
    case StreamMemoryType::Scale:
    case StreamMemoryType::Triad:
        return TestResult::NoImplementation;
    default:
        FATAL_ERROR("Unknown StreamMemoryType");
    }

    ExtensionProperties extensionProperties = ExtensionProperties::create().setMemMapDeviceMemToHostFunctions(true);
    LevelZero levelzero(extensionProperties);
    if (levelzero.memMapDeviceMemToHost == nullptr) {
        return TestResult::DriverFunctionNotFound;
    }

    ze_memory_compression_hints_ext_desc_t uncompressedHint{ZE_STRUCTURE_TYPE_MEMORY_COMPRESSION_HINTS_EXT_DESC};
    uncompressedHint.flags = ZE_MEMORY_COMPRESSION_HINTS_EXT_FLAG_UNCOMPRESSED;

    ze_external_memory_export_desc_t exportDesc{ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_EXPORT_DESC};
    exportDesc.pNext = &uncompressedHint;
    exportDesc.flags = ZE_EXTERNAL_MEMORY_TYPE_FLAG_DMA_BUF;

    // The host can only alias an allocation the driver did not compress, and which descriptor
    // achieves that differs between drivers, so the plain allocation is only the first candidate.
    void *const allocationDescriptors[] = {nullptr, &uncompressedHint, &exportDesc};

    void *deviceBuffer = nullptr;
    void *mappedBuffer = nullptr;
    ze_result_t mapResult = ZE_RESULT_SUCCESS;
    for (void *descriptor : allocationDescriptors) {
        ze_device_mem_alloc_desc_t deviceAllocationDesc{ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC};
        deviceAllocationDesc.pNext = descriptor;
        if (zeMemAllocDevice(levelzero.context, &deviceAllocationDesc, arguments.size, 0, levelzero.device, &deviceBuffer) != ZE_RESULT_SUCCESS) {
            deviceBuffer = nullptr;
            continue;
        }

        mapResult = levelzero.memMapDeviceMemToHost(levelzero.context, deviceBuffer, &mappedBuffer, nullptr);
        if (mapResult == ZE_RESULT_SUCCESS && mappedBuffer != nullptr) {
            break;
        }

        ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, deviceBuffer));
        deviceBuffer = nullptr;
        mappedBuffer = nullptr;

        if (mapResult == ZE_RESULT_ERROR_INCOMPATIBLE_RESOURCE) {
            continue;
        }
        ASSERT_ZE_RESULT_SUCCESS(mapResult);
    }

    if (mappedBuffer == nullptr) {
        DEVELOPER_WARNING_IF(mapResult == ZE_RESULT_ERROR_INCOMPATIBLE_RESOURCE,
                             "device allocation is not host mappable (ZE_RESULT_ERROR_INCOMPATIBLE_RESOURCE) for any of the tried allocation descriptors");
        return TestResult::DeviceNotCapable;
    }

    void *hostBuffer = nullptr;
    ze_host_mem_alloc_desc_t hostAllocationDesc{ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC};
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocHost(levelzero.context, &hostAllocationDesc, arguments.size, 0, &hostBuffer));

    const bool writeToDevice = arguments.type == StreamMemoryType::Write;
    void *destination = writeToDevice ? mappedBuffer : hostBuffer;
    void *source = writeToDevice ? hostBuffer : mappedBuffer;

    for (auto i = 0u; i < arguments.iterations; i++) {
        Timer timer;
        timer.measureStart();
        std::memcpy(destination, source, arguments.size);
        if (writeToDevice && arguments.fenceWrites) {
            std::atomic_thread_fence(std::memory_order_seq_cst);
        }
        timer.measureEnd();
        statistics.pushValue(timer.get(), arguments.size, typeSelector.getUnit(), typeSelector.getType());
    }

    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, hostBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, deviceBuffer));

    return TestResult::Success;
}

static RegisterTestCaseImplementation<HostMappedMemory> registerTestCase(run, Api::L0);
