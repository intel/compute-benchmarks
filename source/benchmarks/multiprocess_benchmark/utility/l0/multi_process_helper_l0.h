/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/utility/hex_helper.h"

#include "utility/multi_process_helper.h"

#include <unordered_map>

struct MultiProcessHelperL0 : MultiProcessHelper {
    static ze_result_t getSubDevicesForExecution(L0::LevelZero &levelzero,
                                                 DeviceSelection subDevices,
                                                 size_t processesPerDevice,
                                                 std::vector<DeviceSelection> &outSubDevicesForExecution) {
        const uint32_t actualSubDevicesCount = static_cast<uint32_t>(levelzero.getSubDevicesCount());
        outSubDevicesForExecution = MultiProcessHelper::getSubDevicesForExecution(subDevices, processesPerDevice, actualSubDevicesCount);
        return ZE_RESULT_SUCCESS;
    }

    struct BufferForSubDevice {
        BufferForSubDevice() = default;
        BufferForSubDevice(const BufferForSubDevice &) = delete;
        BufferForSubDevice &operator=(const BufferForSubDevice &) = delete;
        BufferForSubDevice(BufferForSubDevice &&) = default;
        BufferForSubDevice &operator=(BufferForSubDevice &&) = default;

        void *buffer = {};
        size_t sizeForOneProcess = {};
        size_t totalSize = {};
        ze_ipc_mem_handle_t ipcHandle = {};
        int fileDescriptor;

        std::string getSerializedIpcHandle() const {
            return HexHelper::toHex(reinterpret_cast<const uint8_t *>(this->ipcHandle.data), ZE_MAX_IPC_HANDLE_SIZE);
        }

        size_t getNextOffset() {
            return sizeForOneProcess * (currentProcessesCount++);
        }

      private:
        size_t currentProcessesCount = {};
    };

    static ze_result_t allocateSharedBuffersForSubDevices(L0::LevelZero &levelzero,
                                                          DeviceSelection subDevices,
                                                          size_t processesPerSubDevice,
                                                          size_t workgroupsPerProcess,
                                                          size_t threadsPerWorkgroup,
                                                          std::unordered_map<DeviceSelection, BufferForSubDevice> &outBuffersForSubDevices) {
        for (DeviceSelection subDevice : DeviceSelectionHelper::split(subDevices)) {
            BufferForSubDevice bufferForSubDevice = {};
            bufferForSubDevice.sizeForOneProcess = workgroupsPerProcess * threadsPerWorkgroup * sizeof(uint32_t);
            bufferForSubDevice.totalSize = bufferForSubDevice.sizeForOneProcess * processesPerSubDevice;

            ZE_RESULT_SUCCESS_OR_RETURN(L0::UsmHelper::allocateIpcExportable(UsmMemoryPlacement::Device, levelzero, levelzero.getDevice(subDevice), bufferForSubDevice.totalSize, &bufferForSubDevice.buffer));

            ZE_RESULT_SUCCESS_OR_RETURN(zeMemGetIpcHandle(levelzero.context, bufferForSubDevice.buffer, &bufferForSubDevice.ipcHandle));
            bufferForSubDevice.fileDescriptor = *reinterpret_cast<int *>(bufferForSubDevice.ipcHandle.data);

            outBuffersForSubDevices[subDevice] = std::move(bufferForSubDevice);
        }

        return ZE_RESULT_SUCCESS;
    }
};
