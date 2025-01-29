/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/configuration.h"
#include "framework/l0/context_properties.h"
#include "framework/l0/extension_properties.h"
#include "framework/l0/queue_properties.h"
#include "framework/l0/utility/error.h"
#include "framework/l0/utility/queue_families_helper.h"
#include "framework/utility/timer.h"

#include <level_zero/ze_api.h>

namespace L0 {
struct ImportHostPointerExtension {
    L0ImportExternalPointer importExternalPointer = nullptr;
    L0ReleaseImportedPointer releaseExternalPointer = nullptr;
    L0GetHostPointerBaseAddress getHostPointerBaseAddress = nullptr;
};

// Class handles regular LevelZero boilerplate code, such as querying driver handle, devices, creating contexts,
// and queues. It is configurable by the QueueProperties and ContextProperties objects, allowing to perform the
// setup in a different way than usual. Default constructor always creates both the context and the queue.
//
// LevelZero performs it's own cleanup.
struct LevelZero {
    // Public fields, accessible in benchmarks
    const size_t driverIndex;
    const size_t rootDeviceIndex;
    ze_driver_handle_t driver{};              // Driver instance, always present
    ze_device_handle_t device{};              // Default device, it is not present if multiple devices were selected in ContextSelection
    ze_context_handle_t context{};            // Context, created by default, can be disabled
    ze_command_queue_handle_t commandQueue{}; // Queue, created by default, can be disabled.
    ze_command_queue_desc_t commandQueueDesc{};
    ze_device_handle_t commandQueueDevice{};
    size_t commandQueueMaxFillSize{};
    ImportHostPointerExtension importHostPointer{};
    L0CounterBasedEventCreate2 counterBasedEventCreate2 = nullptr;

    // Constructors, destructor
    LevelZero() : LevelZero(QueueProperties::create()) {}
    LevelZero(const QueueProperties &queueProperties) : LevelZero(queueProperties, ContextProperties::create()) {}
    LevelZero(const ContextProperties &contextProperties) : LevelZero(QueueProperties::create(), contextProperties) {}
    LevelZero(const QueueProperties &queueProperties, const ContextProperties &contextProperties)
        : LevelZero(queueProperties, contextProperties, ExtensionProperties::create()) {}
    LevelZero(const QueueProperties &queueProperties, const ContextProperties &contextProperties,
              const ExtensionProperties &extensionProperties);
    LevelZero(const ExtensionProperties &extensionProperties) : LevelZero(QueueProperties::create(),
                                                                          ContextProperties::create(), extensionProperties) {}
    ~LevelZero() noexcept(false);

    // Returns how many subDevices has been created. Will return 0, if no subDevices were specified in ContextProperties or
    // QueueProperties.
    size_t getSubDevicesCount() const { return subDevices.size(); }

    // Create LevelZero context
    ze_context_handle_t createContext(const ContextProperties &contextProperties);

    // Creates queue with given properties. These methods aren't needed to be called by the user in scenarios with only one queue.
    // Queue will be created by default, unless disabled in QueueProperties. These methods allow the user to create
    // additional queues. Queues are tracked internally and will be released automatically.
    QueueFamiliesHelper::QueueDesc createQueue(const QueueProperties &queueProperties);
    ze_command_queue_handle_t createQueue(ze_device_handle_t device, ze_command_queue_desc_t desc);

    // Returns device for given DeviceSelection. Getting multiple devices at once, e.g. Tile0|Tile1 is forbidden.
    ze_device_handle_t getDevice(DeviceSelection deviceSelection) const;

    // Utility methods for L0 getter functions
    ze_driver_ipc_properties_t getIpcProperties() const {
        ze_driver_ipc_properties_t ipcProperties{ZE_STRUCTURE_TYPE_DRIVER_IPC_PROPERTIES};
        EXPECT_ZE_RESULT_SUCCESS(zeDriverGetIpcProperties(driver, &ipcProperties));
        return ipcProperties;
    }
    ze_device_properties_t getDeviceProperties() const { return getDeviceProperties(this->device); }
    ze_device_properties_t getDeviceProperties(DeviceSelection deviceSelection) const { return getDeviceProperties(getDevice(deviceSelection)); }
    ze_device_properties_t getDeviceProperties(ze_device_handle_t deviceHandle) const {
        ze_device_properties_t deviceProperties{ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES};
        EXPECT_ZE_RESULT_SUCCESS(zeDeviceGetProperties(deviceHandle, &deviceProperties));
        return deviceProperties;
    }
    ze_device_compute_properties_t getDeviceComputeProperties() const { return getDeviceComputeProperties(this->device); }
    ze_device_compute_properties_t getDeviceComputeProperties(DeviceSelection deviceSelection) const { return getDeviceComputeProperties(getDevice(deviceSelection)); }
    ze_device_compute_properties_t getDeviceComputeProperties(ze_device_handle_t deviceHandle) const {
        ze_device_compute_properties_t deviceComputeProperties{ZE_STRUCTURE_TYPE_DEVICE_COMPUTE_PROPERTIES};
        EXPECT_ZE_RESULT_SUCCESS(zeDeviceGetComputeProperties(deviceHandle, &deviceComputeProperties));
        return deviceComputeProperties;
    }

    ze_mutable_command_list_exp_properties_t getDeviceMclProperties() const { return getDeviceMclProperties(this->device); }
    ze_mutable_command_list_exp_properties_t getDeviceMclProperties(DeviceSelection deviceSelection) const { return getDeviceMclProperties(getDevice(deviceSelection)); }
    ze_mutable_command_list_exp_properties_t getDeviceMclProperties(ze_device_handle_t deviceHandle) const;

    bool isMclExtensionAvailable(uint32_t major, uint32_t minor) const;

    uint64_t getTimerResolution(DeviceSelection deviceSelection) const { return getDeviceProperties(deviceSelection).timerResolution; }
    uint64_t getTimerResolution(ze_device_handle_t deviceHandle) const { return getDeviceProperties(deviceHandle).timerResolution; }

    uint32_t getTimestampValidBits(DeviceSelection deviceSelection) const { return getDeviceProperties(deviceSelection).timestampValidBits; }
    uint32_t getTimestampValidBits(ze_device_handle_t deviceHandle) const { return getDeviceProperties(deviceHandle).timestampValidBits; }

    uint32_t getKernelTimestampValidBits(DeviceSelection deviceSelection) const { return getDeviceProperties(deviceSelection).kernelTimestampValidBits; }
    uint32_t getKernelTimestampValidBits(ze_device_handle_t deviceHandle) const { return getDeviceProperties(deviceHandle).kernelTimestampValidBits; }
    uint64_t getKernelTimestampValidBitsMask(ze_device_handle_t deviceHandle) const {
        return static_cast<uint64_t>(((1ull << getDeviceProperties(deviceHandle).kernelTimestampValidBits) - 1ull));
    }

    std::chrono::nanoseconds getAbsoluteSubmissionTime(uint64_t truncatedKernelStartTimestamp,
                                                       uint64_t truncatedDeviceEnqueueTimestamp,
                                                       const uint64_t timerResolution) {
        const uint64_t kernelTimestampValidBitsMask = this->getKernelTimestampValidBitsMask(this->device);
        std::chrono::nanoseconds submissionTime{};
        if (truncatedKernelStartTimestamp > truncatedDeviceEnqueueTimestamp) {
            submissionTime =
                std::chrono::nanoseconds((truncatedKernelStartTimestamp - truncatedDeviceEnqueueTimestamp) * timerResolution);
        } else {
            submissionTime =
                std::chrono::nanoseconds(((kernelTimestampValidBitsMask + 1ull) + truncatedKernelStartTimestamp - truncatedDeviceEnqueueTimestamp) * timerResolution);
        }

        return submissionTime;
    }

    void initializeExtension(const ExtensionProperties &extensionProperties);

    // Queriers subDevices of the root device and creates them if any. This method is only called when it's necessary, i.e. user
    // specified some subDevices in ContextProperties
    void createSubDevices(bool requireSuccess, bool fakeSubDeviceAllowed);

    std::vector<ze_device_handle_t> rootDevices{};

  private:
    // Internal fields managed by the LevelZero class
    ze_device_handle_t rootDevice{};
    std::vector<ze_device_handle_t> subDevices{};
    std::vector<ze_command_queue_handle_t> commandQueues{};
};
} // namespace L0

using namespace L0;
