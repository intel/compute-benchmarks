/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "levelzero.h"

#include "framework/l0/utility/queue_families_helper.h"

namespace L0 {
LevelZero::LevelZero(const QueueProperties &queueProperties, const ContextProperties &contextProperties,
                     const ExtensionProperties &extensionProperties)
    : driverIndex(Configuration::get().l0DriverIndex),
      rootDeviceIndex(Configuration::get().l0DeviceIndex) {
    EXPECT_ZE_RESULT_SUCCESS(zeInit(ZE_INIT_FLAG_GPU_ONLY));

    // Get driver
    uint32_t driverCount = 0;
    EXPECT_ZE_RESULT_SUCCESS(zeDriverGet(&driverCount, nullptr));
    if (driverIndex >= driverCount) {
        FATAL_ERROR("Invalid LevelZero driver index. driverIndex=", driverIndex, " driverCount=", driverCount);
    }
    auto drivers = std::make_unique<ze_driver_handle_t[]>(driverCount);
    EXPECT_ZE_RESULT_SUCCESS(zeDriverGet(&driverCount, drivers.get()));
    this->driver = drivers[driverIndex];

    // Create root device
    uint32_t deviceCount = 0;
    EXPECT_ZE_RESULT_SUCCESS(zeDeviceGet(driver, &deviceCount, nullptr));
    if (rootDeviceIndex >= deviceCount) {
        FATAL_ERROR("Invalid LevelZero device index. deviceIndex=", rootDeviceIndex, " deviceCount=", deviceCount);
    }
    rootDevices.resize(deviceCount);
    EXPECT_ZE_RESULT_SUCCESS(zeDeviceGet(driver, &deviceCount, rootDevices.data()));
    this->rootDevice = rootDevices[rootDeviceIndex];

    // Create subDevices if needed
    if (DeviceSelectionHelper::hasAnySubDevice(contextProperties.deviceSelection)) {
        this->createSubDevices(contextProperties.requireCreationSuccess, contextProperties.fakeSubDeviceAllowed);
        const auto requiredSubDevicesCount = DeviceSelectionHelper::getMaxSubDeviceIndex(contextProperties.deviceSelection) + 1;
        if (this->subDevices.size() < requiredSubDevicesCount) {
            return;
        }
    }

    // Set the default device
    if (DeviceSelectionHelper::hasSingleDevice(contextProperties.deviceSelection)) {
        this->device = getDevice(contextProperties.deviceSelection);
    }

    // Create context on the default device
    this->context = createContext(contextProperties);
    if (this->context == nullptr) {
        return;
    }

    // Create queue
    QueueFamiliesHelper::QueueDesc queueDesc = createQueue(queueProperties);
    this->commandQueue = queueDesc.queue;
    this->commandQueueDesc = queueDesc.desc;
    this->commandQueueDevice = queueDesc.family.device;
    this->commandQueueMaxFillSize = queueDesc.family.maxFillSize;

    initializeImportHostPointerExtension(extensionProperties);
}

LevelZero::~LevelZero() noexcept(false) {
    for (auto &queue : commandQueues) {
        EXPECT_ZE_RESULT_SUCCESS(zeCommandQueueDestroy(queue));
    }
    if (context != nullptr) {
        EXPECT_ZE_RESULT_SUCCESS(zeContextDestroy(context));
    }
}

ze_device_handle_t LevelZero::getDevice(DeviceSelection deviceSelection) const {
    FATAL_ERROR_IF(DeviceSelectionHelper::hasHost(deviceSelection), "Cannot get ze_device_handle_t for host");
    FATAL_ERROR_UNLESS(DeviceSelectionHelper::hasSingleDevice(deviceSelection), "Cannot get multiple devices");
    if (deviceSelection == DeviceSelection::Root) {
        return this->rootDevice;
    }

    const auto subDeviceIndex = DeviceSelectionHelper::getSubDeviceIndex(deviceSelection);
    FATAL_ERROR_UNLESS((subDeviceIndex < this->subDevices.size()), "Invalid subDevice index");
    return this->subDevices[subDeviceIndex];
}

void LevelZero::createSubDevices(bool requireSuccess, bool fakeSubDeviceAllowed) {
    subDevices.clear();

    uint32_t numSubDevices{};
    EXPECT_ZE_RESULT_SUCCESS(zeDeviceGetSubDevices(this->rootDevice, &numSubDevices, nullptr));
    if (numSubDevices > 0) {
        subDevices.resize(numSubDevices);
        EXPECT_ZE_RESULT_SUCCESS(zeDeviceGetSubDevices(this->rootDevice, &numSubDevices, subDevices.data()));
    } else if (fakeSubDeviceAllowed) {
        subDevices.push_back(rootDevice);
    } else if (requireSuccess) {
        FATAL_ERROR("SubDevice was selected, but device has 0 subDevices");
    }
}

ze_context_handle_t LevelZero::createContext(const ContextProperties &contextProperties) {
    if (!contextProperties.createContext) {
        return nullptr;
    }

    const ze_context_desc_t contextDesc{ZE_STRUCTURE_TYPE_CONTEXT_DESC};
    ze_context_handle_t contextHandle;
    EXPECT_ZE_RESULT_SUCCESS(zeContextCreate(driver, &contextDesc, &contextHandle));
    return contextHandle;
}

QueueFamiliesHelper::QueueDesc LevelZero::createQueue(const QueueProperties &queueProperties) {
    if (!queueProperties.createQueue) {
        return {};
    }

    // Get device
    const ze_device_handle_t deviceForQueue = getDevice(queueProperties.deviceSelection);

    // Get queue info, which matches our requirements passed in QueueProperties
    auto queueDesc = QueueFamiliesHelper::getPropertiesForSelectingEngine(deviceForQueue, queueProperties.selectedEngine);
    if (queueDesc == nullptr) {
        FATAL_ERROR_IF(queueProperties.requireCreationSuccess, "Device does not support such queue");
        return {};
    }

    // Create
    queueDesc->queue = createQueue(deviceForQueue, queueDesc->desc);
    return *queueDesc;
}

ze_command_queue_handle_t LevelZero::createQueue(ze_device_handle_t deviceHandle, ze_command_queue_desc_t desc) {
    ze_command_queue_handle_t queue = {};
    EXPECT_ZE_RESULT_SUCCESS(zeCommandQueueCreate(this->context, deviceHandle, &desc, &queue));
    this->commandQueues.push_back(queue);
    return queue;
}

void LevelZero::initializeImportHostPointerExtension(const ExtensionProperties &extensionProperties) {
    if (!extensionProperties.getImportHostPointerFunctions) {
        return;
    }

    EXPECT_ZE_RESULT_SUCCESS(
        zeDriverGetExtensionFunctionAddress(this->driver,
                                            "zexDriverImportExternalPointer",
                                            reinterpret_cast<void **>(&this->importHostPointer.importExternalPointer)));
    FATAL_ERROR_IF(this->importHostPointer.importExternalPointer == nullptr, "zexDriverImportExternalPointer retrieved nullptr");
    EXPECT_ZE_RESULT_SUCCESS(
        zeDriverGetExtensionFunctionAddress(this->driver,
                                            "zexDriverReleaseImportedPointer",
                                            reinterpret_cast<void **>(&this->importHostPointer.releaseExternalPointer)));
    FATAL_ERROR_IF(this->importHostPointer.releaseExternalPointer == nullptr, "zexDriverReleaseImportedPointer retrieved nullptr");
    EXPECT_ZE_RESULT_SUCCESS(
        zeDriverGetExtensionFunctionAddress(this->driver,
                                            "zexDriverGetHostPointerBaseAddress",
                                            reinterpret_cast<void **>(&this->importHostPointer.getHostPointerBaseAddress)));
    FATAL_ERROR_IF(this->importHostPointer.getHostPointerBaseAddress == nullptr, "zexDriverGetHostPointerBaseAddress retrieved nullptr");
}

} // namespace L0
