/*
 * Copyright (C) 2022-2026 Intel Corporation
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

    initializeExtension(extensionProperties);
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
    auto desc = queueDesc->desc;
    desc.priority = queueProperties.priority;

    // Create
    queueDesc->queue = createQueue(deviceForQueue, desc);
    return *queueDesc;
}

ze_command_queue_handle_t LevelZero::createQueue(ze_device_handle_t deviceHandle, ze_command_queue_desc_t desc) {
    ze_command_queue_handle_t queue = {};
    EXPECT_ZE_RESULT_SUCCESS(zeCommandQueueCreate(this->context, deviceHandle, &desc, &queue));
    this->commandQueues.push_back(queue);
    return queue;
}

void LevelZero::initializeExtension(const ExtensionProperties &extensionProperties) {
    if (extensionProperties.getImportHostPointerFunctions) {
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

    if (extensionProperties.getCounterBasedCreateFunctions) {
        EXPECT_ZE_RESULT_SUCCESS(
            zeDriverGetExtensionFunctionAddress(this->driver,
                                                "zexCounterBasedEventCreate2",
                                                reinterpret_cast<void **>(&this->counterBasedEventCreate2)));
        FATAL_ERROR_IF(this->counterBasedEventCreate2 == nullptr, "zexCounterBasedEventCreate2 retrieved nullptr");
    }

    if (extensionProperties.getGraphFunctions) {
        EXPECT_ZE_RESULT_SUCCESS(
            zeDriverGetExtensionFunctionAddress(this->driver,
                                                "zeGraphCreateExp",
                                                reinterpret_cast<void **>(&this->graphExtension.graphCreate)));
        FATAL_ERROR_IF(this->graphExtension.graphCreate == nullptr, "zeGraphCreateExp retrieved nullptr");

        EXPECT_ZE_RESULT_SUCCESS(
            zeDriverGetExtensionFunctionAddress(this->driver,
                                                "zeCommandListBeginCaptureIntoGraphExp",
                                                reinterpret_cast<void **>(&this->graphExtension.commandListBeginCaptureIntoGraph)));
        FATAL_ERROR_IF(this->graphExtension.commandListBeginCaptureIntoGraph == nullptr, "zeCommandListBeginCaptureIntoGraphExp retrieved nullptr");

        EXPECT_ZE_RESULT_SUCCESS(
            zeDriverGetExtensionFunctionAddress(this->driver,
                                                "zeCommandListEndGraphCaptureExp",
                                                reinterpret_cast<void **>(&this->graphExtension.commandListEndGraphCapture)));
        FATAL_ERROR_IF(this->graphExtension.commandListEndGraphCapture == nullptr, "zeCommandListEndGraphCaptureExp retrieved nullptr");

        EXPECT_ZE_RESULT_SUCCESS(
            zeDriverGetExtensionFunctionAddress(this->driver,
                                                "zeCommandListInstantiateGraphExp",
                                                reinterpret_cast<void **>(&this->graphExtension.commandListInstantiateGraph)));
        FATAL_ERROR_IF(this->graphExtension.commandListInstantiateGraph == nullptr, "zeCommandListInstantiateGraphExp retrieved nullptr");

        EXPECT_ZE_RESULT_SUCCESS(
            zeDriverGetExtensionFunctionAddress(this->driver,
                                                "zeCommandListAppendGraphExp",
                                                reinterpret_cast<void **>(&this->graphExtension.commandListAppendGraph)));
        FATAL_ERROR_IF(this->graphExtension.commandListAppendGraph == nullptr, "zeCommandListAppendGraphExp retrieved nullptr");

        EXPECT_ZE_RESULT_SUCCESS(
            zeDriverGetExtensionFunctionAddress(this->driver,
                                                "zeGraphDestroyExp",
                                                reinterpret_cast<void **>(&this->graphExtension.graphDestroy)));
        FATAL_ERROR_IF(this->graphExtension.graphDestroy == nullptr, "zeGraphDestroyExp retrieved nullptr");

        EXPECT_ZE_RESULT_SUCCESS(
            zeDriverGetExtensionFunctionAddress(this->driver,
                                                "zeExecutableGraphDestroyExp",
                                                reinterpret_cast<void **>(&this->graphExtension.executableGraphDestroy)));
        FATAL_ERROR_IF(this->graphExtension.executableGraphDestroy == nullptr, "zeExecutableGraphDestroyExp retrieved nullptr");
    }
}

ze_mutable_command_list_exp_properties_t LevelZero::getDeviceMclProperties(ze_device_handle_t deviceHandle) const {
    ze_mutable_command_list_exp_properties_t mclProperties{ZE_STRUCTURE_TYPE_MUTABLE_COMMAND_LIST_EXP_PROPERTIES};
    ze_device_properties_t deviceProperties{ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES};
    deviceProperties.pNext = &mclProperties;

    EXPECT_ZE_RESULT_SUCCESS(zeDeviceGetProperties(deviceHandle, &deviceProperties));
    return mclProperties;
}

bool LevelZero::isMclExtensionAvailable(uint32_t major, uint32_t minor) const {
    const std::string name = "ZE_experimental_mutable_command_list";
    const uint32_t version = ZE_MAKE_VERSION(major, minor);

    uint32_t extensionsCount = 0;
    EXPECT_ZE_RESULT_SUCCESS(zeDriverGetExtensionProperties(driver, &extensionsCount, nullptr));

    std::vector<ze_driver_extension_properties_t> driverExtensions(extensionsCount);
    EXPECT_ZE_RESULT_SUCCESS(zeDriverGetExtensionProperties(driver, &extensionsCount, driverExtensions.data()));
    for (uint32_t i = 0; i < extensionsCount; i++) {
        auto queryName = driverExtensions[i].name;
        if (strncmp(queryName, name.c_str(), name.size()) == 0) {
            auto queryVersion = driverExtensions[i].version;
            if (queryVersion >= version) {
                return true;
            }
        }
    }
    return false;
}

} // namespace L0
