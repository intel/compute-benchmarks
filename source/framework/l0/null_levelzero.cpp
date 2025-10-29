/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifdef NULL_L0

#include "level_zero/zes_api.h"
#include "levelzero.h"

#define FAIL_NOT_IMPLEMENTED                                             \
    std::cerr << __func__ << " not implemented in null_levelzero.cpp\n"; \
    abort();

// Multi versions (same pattern, all args unused, return success)
#define ZE_MOCK_SUCCESS(name, ...)                          \
    ZE_APIEXPORT ze_result_t ZE_APICALL name(__VA_ARGS__) { \
        return ZE_RESULT_SUCCESS;                           \
    }

#define ZE_MOCK_FAILURE(name, ...)                          \
    ZE_APIEXPORT ze_result_t ZE_APICALL name(__VA_ARGS__) { \
        FAIL_NOT_IMPLEMENTED;                               \
    }

// Extension functions

ZE_APIEXPORT ze_result_t ZE_APICALL null_zeGraphCreateExp(ze_context_handle_t hContext, ze_graph_handle_t *phGraph, void *pNext) {
    (void)hContext;
    (void)pNext;
    if (phGraph)
        *phGraph = reinterpret_cast<ze_graph_handle_t>(0x10);
    return ZE_RESULT_SUCCESS;
}

ZE_MOCK_SUCCESS(null_zeCommandListBeginGraphCaptureExp, ze_command_list_handle_t, void *)
ZE_MOCK_SUCCESS(null_zeCommandListBeginCaptureIntoGraphExp, ze_command_list_handle_t, ze_graph_handle_t, void *)

ZE_APIEXPORT ze_result_t ZE_APICALL null_zeCommandListEndGraphCaptureExp(ze_command_list_handle_t hCommandList, ze_graph_handle_t *phGraph, void *pNext) {
    (void)hCommandList;
    (void)pNext;
    if (phGraph)
        *phGraph = reinterpret_cast<ze_graph_handle_t>(0x11);
    return ZE_RESULT_SUCCESS;
}

ZE_APIEXPORT ze_result_t ZE_APICALL null_zeCommandListInstantiateGraphExp(ze_graph_handle_t hGraph, ze_executable_graph_handle_t *phExecutableGraph, void *pNext) {
    (void)hGraph;
    (void)pNext;
    if (phExecutableGraph)
        *phExecutableGraph = reinterpret_cast<ze_executable_graph_handle_t>(0x12);
    return ZE_RESULT_SUCCESS;
}

ZE_MOCK_SUCCESS(null_zeCommandListAppendGraphExp, ze_command_list_handle_t, ze_executable_graph_handle_t, void *, ze_event_handle_t, uint32_t, ze_event_handle_t *)
ZE_MOCK_SUCCESS(null_zeGraphDestroyExp, ze_graph_handle_t)
ZE_MOCK_SUCCESS(null_zeExecutableGraphDestroyExp, ze_executable_graph_handle_t)
ZE_MOCK_SUCCESS(null_zeCommandListIsGraphCaptureEnabledExp, ze_command_list_handle_t)
ZE_MOCK_SUCCESS(null_zeGraphIsEmptyExp, ze_graph_handle_t)
ZE_MOCK_SUCCESS(null_zeGraphDumpContentsExp, ze_graph_handle_t, const char *, void *)

ZE_APIEXPORT ze_result_t ZE_APICALL
zeDriverGetExtensionFunctionAddress(
    ze_driver_handle_t hDriver,
    const char *name,
    void **ppFunctionAddress) {
    (void)hDriver;
    if (!name || !ppFunctionAddress)
        return ZE_RESULT_ERROR_INVALID_ARGUMENT;

    struct NameAddr {
        const char *fname;
        void *fptr;
    };
    static const NameAddr table[] = {
        {"zeGraphCreateExp", (void *)&null_zeGraphCreateExp},
        {"zeCommandListBeginGraphCaptureExp", (void *)&null_zeCommandListBeginGraphCaptureExp},
        {"zeCommandListBeginCaptureIntoGraphExp", (void *)&null_zeCommandListBeginCaptureIntoGraphExp},
        {"zeCommandListEndGraphCaptureExp", (void *)&null_zeCommandListEndGraphCaptureExp},
        {"zeCommandListInstantiateGraphExp", (void *)&null_zeCommandListInstantiateGraphExp},
        {"zeCommandListAppendGraphExp", (void *)&null_zeCommandListAppendGraphExp},
        {"zeGraphDestroyExp", (void *)&null_zeGraphDestroyExp},
        {"zeExecutableGraphDestroyExp", (void *)&null_zeExecutableGraphDestroyExp},
        {"zeCommandListIsGraphCaptureEnabledExp", (void *)&null_zeCommandListIsGraphCaptureEnabledExp},
        {"zeGraphIsEmptyExp", (void *)&null_zeGraphIsEmptyExp},
        {"zeGraphDumpContentsExp", (void *)&null_zeGraphDumpContentsExp},
    };

    for (const auto &entry : table) {
        if (strcmp(name, entry.fname) == 0) {
            *ppFunctionAddress = entry.fptr;
            return ZE_RESULT_SUCCESS;
        }
    }

    std::cerr << __func__ << " not implemented case of " << name << " in null_levelzero.cpp\n";
    abort();
}

ZE_MOCK_SUCCESS(zeInit, ze_init_flags_t)

ZE_APIEXPORT ze_result_t ZE_APICALL zeDriverGet(uint32_t *pCount,
                                                ze_driver_handle_t *phDrivers) {
    if (*pCount == 0)
        *pCount = 1;
    else if (phDrivers != nullptr)
        phDrivers[0] = (ze_driver_handle_t)0x1;
    return ZE_RESULT_SUCCESS;
}

ZE_MOCK_SUCCESS(zeInitDrivers, uint32_t *, ze_driver_handle_t *, ze_init_driver_type_desc_t *)
ZE_MOCK_SUCCESS(zeDriverGetApiVersion, ze_driver_handle_t, ze_api_version_t *)
ZE_MOCK_FAILURE(zeDriverGetProperties, ze_driver_handle_t, ze_driver_properties_t *)
ZE_MOCK_FAILURE(zeDriverGetIpcProperties, ze_driver_handle_t, ze_driver_ipc_properties_t *)
ZE_MOCK_SUCCESS(zeDriverGetExtensionProperties, ze_driver_handle_t, uint32_t *, ze_driver_extension_properties_t *)
// zeDriverGetExtensionFunctionAddress is defined earlier
ZE_MOCK_SUCCESS(zeDriverGetLastErrorDescription, ze_driver_handle_t, const char **)

ZE_APIEXPORT ze_result_t ZE_APICALL zeDeviceGet(ze_driver_handle_t hDriver,
                                                uint32_t *pCount,
                                                ze_device_handle_t *phDevices) {
    (void)hDriver;
    if (*pCount == 0)
        *pCount = 1;
    else if (phDevices != nullptr)
        phDevices[0] = reinterpret_cast<ze_device_handle_t>(0x1);
    return ZE_RESULT_SUCCESS;
}

ZE_MOCK_SUCCESS(zeDeviceGetRootDevice, ze_device_handle_t, ze_device_handle_t *)

ZE_APIEXPORT ze_result_t ZE_APICALL
zeDeviceGetSubDevices(
    ze_device_handle_t hDevice,
    uint32_t *pCount,
    ze_device_handle_t *phSubdevices) {
    (void)hDevice;
    (void)phSubdevices;
    *pCount = 0;
    return ZE_RESULT_SUCCESS;
}

ZE_MOCK_FAILURE(zeDeviceGetProperties, ze_device_handle_t, ze_device_properties_t *)
ZE_MOCK_FAILURE(zeDeviceGetComputeProperties, ze_device_handle_t, ze_device_compute_properties_t *)
ZE_MOCK_FAILURE(zeDeviceGetModuleProperties, ze_device_handle_t, ze_device_module_properties_t *)

ZE_APIEXPORT ze_result_t ZE_APICALL
zeDeviceGetCommandQueueGroupProperties(
    ze_device_handle_t hDevice,
    uint32_t *pCount,
    ze_command_queue_group_properties_t *pCommandQueueGroupProperties) {
    (void)hDevice;
    if (*pCount == 0) {
        *pCount = 1;
        return ZE_RESULT_SUCCESS;
    }

    *pCount = 1;
    pCommandQueueGroupProperties->stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_GROUP_PROPERTIES;
    pCommandQueueGroupProperties->pNext = NULL;
    pCommandQueueGroupProperties->flags = ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE | ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY;
    pCommandQueueGroupProperties->maxMemoryFillPatternSize = 4096;
    pCommandQueueGroupProperties->numQueues = 1;
    return ZE_RESULT_SUCCESS;
}

ZE_MOCK_FAILURE(zeDeviceGetMemoryProperties, ze_device_handle_t, uint32_t *, ze_device_memory_properties_t *)
ZE_MOCK_SUCCESS(zeDeviceGetMemoryAccessProperties, ze_device_handle_t, ze_device_memory_access_properties_t *)
ZE_MOCK_SUCCESS(zeDeviceGetCacheProperties, ze_device_handle_t, uint32_t *, ze_device_cache_properties_t *)
ZE_MOCK_SUCCESS(zeDeviceGetImageProperties, ze_device_handle_t, ze_device_image_properties_t *)
ZE_MOCK_SUCCESS(zeDeviceGetExternalMemoryProperties, ze_device_handle_t, ze_device_external_memory_properties_t *)
ZE_MOCK_SUCCESS(zeDeviceGetP2PProperties, ze_device_handle_t, ze_device_handle_t, ze_device_p2p_properties_t *)
ZE_MOCK_SUCCESS(zeDeviceCanAccessPeer, ze_device_handle_t, ze_device_handle_t, ze_bool_t *)
ZE_MOCK_SUCCESS(zeDeviceGetStatus, ze_device_handle_t)
ZE_MOCK_SUCCESS(zeDeviceGetGlobalTimestamps, ze_device_handle_t, uint64_t *, uint64_t *)
ZE_MOCK_SUCCESS(zeContextCreate, ze_driver_handle_t, const ze_context_desc_t *, ze_context_handle_t *)
ZE_MOCK_SUCCESS(zeContextCreateEx, ze_driver_handle_t, const ze_context_desc_t *, uint32_t, ze_device_handle_t *, ze_context_handle_t *)
ZE_MOCK_SUCCESS(zeContextDestroy, ze_context_handle_t)
ZE_MOCK_SUCCESS(zeContextGetStatus, ze_context_handle_t)
ZE_MOCK_SUCCESS(zeCommandQueueCreate, ze_context_handle_t, ze_device_handle_t, const ze_command_queue_desc_t *, ze_command_queue_handle_t *)
ZE_MOCK_SUCCESS(zeCommandQueueDestroy, ze_command_queue_handle_t)
ZE_MOCK_SUCCESS(zeCommandQueueExecuteCommandLists, ze_command_queue_handle_t, uint32_t, ze_command_list_handle_t *, ze_fence_handle_t)
ZE_MOCK_SUCCESS(zeCommandQueueSynchronize, ze_command_queue_handle_t, uint64_t)
ZE_MOCK_SUCCESS(zeCommandQueueGetOrdinal, ze_command_queue_handle_t, uint32_t *)
ZE_MOCK_SUCCESS(zeCommandQueueGetIndex, ze_command_queue_handle_t, uint32_t *)
ZE_MOCK_SUCCESS(zeCommandListCreate, ze_context_handle_t, ze_device_handle_t, const ze_command_list_desc_t *, ze_command_list_handle_t *)
ZE_MOCK_SUCCESS(zeCommandListCreateImmediate, ze_context_handle_t, ze_device_handle_t, const ze_command_queue_desc_t *, ze_command_list_handle_t *)
ZE_MOCK_SUCCESS(zeCommandListDestroy, ze_command_list_handle_t)
ZE_MOCK_SUCCESS(zeCommandListClose, ze_command_list_handle_t)
ZE_MOCK_SUCCESS(zeCommandListReset, ze_command_list_handle_t)
ZE_MOCK_SUCCESS(zeCommandListAppendWriteGlobalTimestamp, ze_command_list_handle_t, uint64_t *, ze_event_handle_t, uint32_t, ze_event_handle_t *)
ZE_MOCK_SUCCESS(zeCommandListHostSynchronize, ze_command_list_handle_t, uint64_t)
ZE_MOCK_SUCCESS(zeCommandListGetDeviceHandle, ze_command_list_handle_t, ze_device_handle_t *)
ZE_MOCK_SUCCESS(zeCommandListGetContextHandle, ze_command_list_handle_t, ze_context_handle_t *)
ZE_MOCK_SUCCESS(zeCommandListGetOrdinal, ze_command_list_handle_t, uint32_t *)
ZE_MOCK_SUCCESS(zeCommandListImmediateGetIndex, ze_command_list_handle_t, uint32_t *)
ZE_MOCK_SUCCESS(zeCommandListIsImmediate, ze_command_list_handle_t, ze_bool_t *)
ZE_MOCK_SUCCESS(zeCommandListAppendBarrier, ze_command_list_handle_t, ze_event_handle_t, uint32_t, ze_event_handle_t *)
ZE_MOCK_SUCCESS(zeCommandListAppendMemoryRangesBarrier, ze_command_list_handle_t, uint32_t, const size_t *, const void **, ze_event_handle_t, uint32_t, ze_event_handle_t *)
ZE_MOCK_SUCCESS(zeContextSystemBarrier, ze_context_handle_t, ze_device_handle_t)
ZE_MOCK_SUCCESS(zeCommandListAppendMemoryCopy, ze_command_list_handle_t, void *, const void *, size_t, ze_event_handle_t, uint32_t, ze_event_handle_t *)
ZE_MOCK_SUCCESS(zeCommandListAppendMemoryFill, ze_command_list_handle_t, void *, const void *, size_t, size_t, ze_event_handle_t, uint32_t, ze_event_handle_t *)
ZE_MOCK_SUCCESS(zeCommandListAppendMemoryCopyRegion, ze_command_list_handle_t, void *, const ze_copy_region_t *, uint32_t, uint32_t, const void *, const ze_copy_region_t *, uint32_t, uint32_t, ze_event_handle_t, uint32_t, ze_event_handle_t *)
ZE_MOCK_SUCCESS(zeCommandListAppendMemoryCopyFromContext, ze_command_list_handle_t, void *, ze_context_handle_t, const void *, size_t, ze_event_handle_t, uint32_t, ze_event_handle_t *)
ZE_MOCK_SUCCESS(zeCommandListAppendImageCopy, ze_command_list_handle_t, ze_image_handle_t, ze_image_handle_t, ze_event_handle_t, uint32_t, ze_event_handle_t *)
ZE_MOCK_SUCCESS(zeCommandListAppendImageCopyRegion, ze_command_list_handle_t, ze_image_handle_t, ze_image_handle_t, const ze_image_region_t *, const ze_image_region_t *, ze_event_handle_t, uint32_t, ze_event_handle_t *)
ZE_MOCK_SUCCESS(zeCommandListAppendImageCopyToMemory, ze_command_list_handle_t, void *, ze_image_handle_t, const ze_image_region_t *, ze_event_handle_t, uint32_t, ze_event_handle_t *)
ZE_MOCK_SUCCESS(zeCommandListAppendImageCopyFromMemory, ze_command_list_handle_t, ze_image_handle_t, const void *, const ze_image_region_t *, ze_event_handle_t, uint32_t, ze_event_handle_t *)
ZE_MOCK_SUCCESS(zeCommandListAppendMemoryPrefetch, ze_command_list_handle_t, const void *, size_t)
ZE_MOCK_SUCCESS(zeCommandListAppendMemAdvise, ze_command_list_handle_t, ze_device_handle_t, const void *, size_t, ze_memory_advice_t)
ZE_MOCK_SUCCESS(zeEventPoolCreate, ze_context_handle_t, const ze_event_pool_desc_t *, uint32_t, ze_device_handle_t *, ze_event_pool_handle_t *)
ZE_MOCK_SUCCESS(zeEventPoolDestroy, ze_event_pool_handle_t)
ZE_MOCK_SUCCESS(zeEventCreate, ze_event_pool_handle_t, const ze_event_desc_t *, ze_event_handle_t *)
ZE_MOCK_SUCCESS(zeEventDestroy, ze_event_handle_t)
ZE_MOCK_SUCCESS(zeEventPoolGetIpcHandle, ze_event_pool_handle_t, ze_ipc_event_pool_handle_t *)
ZE_MOCK_SUCCESS(zeEventPoolPutIpcHandle, ze_context_handle_t, ze_ipc_event_pool_handle_t)
ZE_MOCK_SUCCESS(zeEventPoolOpenIpcHandle, ze_context_handle_t, ze_ipc_event_pool_handle_t, ze_event_pool_handle_t *)
ZE_MOCK_SUCCESS(zeEventPoolCloseIpcHandle, ze_event_pool_handle_t)
ZE_MOCK_SUCCESS(zeCommandListAppendSignalEvent, ze_command_list_handle_t, ze_event_handle_t)
ZE_MOCK_SUCCESS(zeCommandListAppendWaitOnEvents, ze_command_list_handle_t, uint32_t, ze_event_handle_t *)
ZE_MOCK_SUCCESS(zeEventHostSignal, ze_event_handle_t)
ZE_MOCK_SUCCESS(zeEventHostSynchronize, ze_event_handle_t, uint64_t)
ZE_MOCK_SUCCESS(zeEventQueryStatus, ze_event_handle_t)
ZE_MOCK_SUCCESS(zeCommandListAppendEventReset, ze_command_list_handle_t, ze_event_handle_t)
ZE_MOCK_SUCCESS(zeEventHostReset, ze_event_handle_t)
ZE_MOCK_SUCCESS(zeEventQueryKernelTimestamp, ze_event_handle_t, ze_kernel_timestamp_result_t *)
ZE_MOCK_SUCCESS(zeCommandListAppendQueryKernelTimestamps, ze_command_list_handle_t, uint32_t, ze_event_handle_t *, void *, const size_t *, ze_event_handle_t, uint32_t, ze_event_handle_t *)
ZE_MOCK_SUCCESS(zeEventGetEventPool, ze_event_handle_t, ze_event_pool_handle_t *)
ZE_MOCK_SUCCESS(zeEventGetSignalScope, ze_event_handle_t, ze_event_scope_flags_t *)
ZE_MOCK_SUCCESS(zeEventGetWaitScope, ze_event_handle_t, ze_event_scope_flags_t *)
ZE_MOCK_SUCCESS(zeEventPoolGetContextHandle, ze_event_pool_handle_t, ze_context_handle_t *)
ZE_MOCK_SUCCESS(zeEventPoolGetFlags, ze_event_pool_handle_t, ze_event_pool_flags_t *)
ZE_MOCK_SUCCESS(zeFenceCreate, ze_command_queue_handle_t, const ze_fence_desc_t *, ze_fence_handle_t *)
ZE_MOCK_SUCCESS(zeFenceDestroy, ze_fence_handle_t)
ZE_MOCK_SUCCESS(zeFenceHostSynchronize, ze_fence_handle_t, uint64_t)
ZE_MOCK_SUCCESS(zeFenceQueryStatus, ze_fence_handle_t)
ZE_MOCK_SUCCESS(zeFenceReset, ze_fence_handle_t)
ZE_MOCK_SUCCESS(zeImageGetProperties, ze_device_handle_t, const ze_image_desc_t *, ze_image_properties_t *)
ZE_MOCK_SUCCESS(zeImageCreate, ze_context_handle_t, ze_device_handle_t, const ze_image_desc_t *, ze_image_handle_t *)
ZE_MOCK_SUCCESS(zeImageDestroy, ze_image_handle_t)
ZE_MOCK_SUCCESS(zeMemAllocShared, ze_context_handle_t, const ze_device_mem_alloc_desc_t *const, const ze_host_mem_alloc_desc_t *const, size_t, size_t, ze_device_handle_t, void **)
ZE_MOCK_SUCCESS(zeMemAllocDevice, ze_context_handle_t, const ze_device_mem_alloc_desc_t *const, size_t, size_t, ze_device_handle_t, void **)
ZE_MOCK_SUCCESS(zeMemAllocHost, ze_context_handle_t, const ze_host_mem_alloc_desc_t *const, size_t, size_t, void **)
ZE_MOCK_SUCCESS(zeMemFree, ze_context_handle_t, void *)
ZE_MOCK_SUCCESS(zeMemGetAllocProperties, ze_context_handle_t, const void *, ze_memory_allocation_properties_t *, ze_device_handle_t *)
ZE_MOCK_SUCCESS(zeMemGetAddressRange, ze_context_handle_t, const void *, void **, size_t *)
ZE_MOCK_SUCCESS(zeMemGetIpcHandle, ze_context_handle_t, const void *, ze_ipc_mem_handle_t *)
ZE_MOCK_SUCCESS(zeMemGetIpcHandleFromFileDescriptorExp, ze_context_handle_t, uint64_t, ze_ipc_mem_handle_t *)
ZE_MOCK_SUCCESS(zeMemGetFileDescriptorFromIpcHandleExp, ze_context_handle_t, ze_ipc_mem_handle_t, uint64_t *)
ZE_MOCK_SUCCESS(zeMemPutIpcHandle, ze_context_handle_t, ze_ipc_mem_handle_t)
ZE_MOCK_SUCCESS(zeMemOpenIpcHandle, ze_context_handle_t, ze_device_handle_t, ze_ipc_mem_handle_t, ze_ipc_memory_flags_t, void **)
ZE_MOCK_SUCCESS(zeMemCloseIpcHandle, ze_context_handle_t, const void *)
ZE_MOCK_SUCCESS(zeMemSetAtomicAccessAttributeExp, ze_context_handle_t, ze_device_handle_t, const void *, size_t, ze_memory_atomic_attr_exp_flags_t)
ZE_MOCK_SUCCESS(zeMemGetAtomicAccessAttributeExp, ze_context_handle_t, ze_device_handle_t, const void *, size_t, ze_memory_atomic_attr_exp_flags_t *)
ZE_MOCK_SUCCESS(zeModuleCreate, ze_context_handle_t, ze_device_handle_t, const ze_module_desc_t *, ze_module_handle_t *, ze_module_build_log_handle_t *)
ZE_MOCK_SUCCESS(zeModuleDestroy, ze_module_handle_t)
ZE_MOCK_SUCCESS(zeModuleDynamicLink, uint32_t, ze_module_handle_t *, ze_module_build_log_handle_t *)
ZE_MOCK_SUCCESS(zeModuleBuildLogDestroy, ze_module_build_log_handle_t)
ZE_MOCK_SUCCESS(zeModuleBuildLogGetString, ze_module_build_log_handle_t, size_t *, char *)
ZE_MOCK_SUCCESS(zeModuleGetNativeBinary, ze_module_handle_t, size_t *, uint8_t *)
ZE_MOCK_SUCCESS(zeModuleGetGlobalPointer, ze_module_handle_t, const char *, size_t *, void **)
ZE_MOCK_SUCCESS(zeModuleGetKernelNames, ze_module_handle_t, uint32_t *, const char **)
ZE_MOCK_SUCCESS(zeModuleGetProperties, ze_module_handle_t, ze_module_properties_t *)
ZE_MOCK_SUCCESS(zeKernelCreate, ze_module_handle_t, const ze_kernel_desc_t *, ze_kernel_handle_t *)
ZE_MOCK_SUCCESS(zeKernelDestroy, ze_kernel_handle_t)
ZE_MOCK_SUCCESS(zeModuleGetFunctionPointer, ze_module_handle_t, const char *, void **)
ZE_MOCK_SUCCESS(zeKernelSetGroupSize, ze_kernel_handle_t, uint32_t, uint32_t, uint32_t)
ZE_MOCK_SUCCESS(zeKernelSuggestGroupSize, ze_kernel_handle_t, uint32_t, uint32_t, uint32_t, uint32_t *, uint32_t *, uint32_t *)
ZE_MOCK_SUCCESS(zeKernelSuggestMaxCooperativeGroupCount, ze_kernel_handle_t, uint32_t *)
ZE_MOCK_SUCCESS(zeKernelSetArgumentValue, ze_kernel_handle_t, uint32_t, size_t, const void *)
ZE_MOCK_SUCCESS(zeKernelSetIndirectAccess, ze_kernel_handle_t, ze_kernel_indirect_access_flags_t)
ZE_MOCK_SUCCESS(zeKernelGetIndirectAccess, ze_kernel_handle_t, ze_kernel_indirect_access_flags_t *)
ZE_MOCK_SUCCESS(zeKernelGetSourceAttributes, ze_kernel_handle_t, uint32_t *, char **)
ZE_MOCK_SUCCESS(zeKernelSetCacheConfig, ze_kernel_handle_t, ze_cache_config_flags_t)
ZE_MOCK_SUCCESS(zeKernelGetProperties, ze_kernel_handle_t, ze_kernel_properties_t *)
ZE_MOCK_SUCCESS(zeKernelGetName, ze_kernel_handle_t, size_t *, char *)
ZE_MOCK_SUCCESS(zeCommandListAppendLaunchKernel, ze_command_list_handle_t, ze_kernel_handle_t, const ze_group_count_t *, ze_event_handle_t, uint32_t, ze_event_handle_t *)
ZE_MOCK_SUCCESS(zeCommandListAppendLaunchCooperativeKernel, ze_command_list_handle_t, ze_kernel_handle_t, const ze_group_count_t *, ze_event_handle_t, uint32_t, ze_event_handle_t *)
ZE_MOCK_SUCCESS(zeCommandListAppendLaunchKernelIndirect, ze_command_list_handle_t, ze_kernel_handle_t, const ze_group_count_t *, ze_event_handle_t, uint32_t, ze_event_handle_t *)
ZE_MOCK_SUCCESS(zeCommandListAppendLaunchMultipleKernelsIndirect, ze_command_list_handle_t, uint32_t, ze_kernel_handle_t *, const uint32_t *, const ze_group_count_t *, ze_event_handle_t, uint32_t, ze_event_handle_t *)
ZE_MOCK_SUCCESS(zeContextMakeMemoryResident, ze_context_handle_t, ze_device_handle_t, void *, size_t)
ZE_MOCK_SUCCESS(zeContextEvictMemory, ze_context_handle_t, ze_device_handle_t, void *, size_t)
ZE_MOCK_SUCCESS(zeContextMakeImageResident, ze_context_handle_t, ze_device_handle_t, ze_image_handle_t)
ZE_MOCK_SUCCESS(zeContextEvictImage, ze_context_handle_t, ze_device_handle_t, ze_image_handle_t)
ZE_MOCK_SUCCESS(zeSamplerCreate, ze_context_handle_t, ze_device_handle_t, const ze_sampler_desc_t *, ze_sampler_handle_t *)
ZE_MOCK_SUCCESS(zeSamplerDestroy, ze_sampler_handle_t)
ZE_MOCK_SUCCESS(zeVirtualMemReserve, ze_context_handle_t, const void *, size_t, void **)
ZE_MOCK_SUCCESS(zeVirtualMemFree, ze_context_handle_t, const void *, size_t)

ZE_APIEXPORT ze_result_t ZE_APICALL zeVirtualMemQueryPageSize(
    ze_context_handle_t hContext,
    ze_device_handle_t hDevice,
    size_t size,
    size_t *pPageSize) {
    (void)hContext;
    (void)hDevice;
    (void)size;
    if (pPageSize)
        *pPageSize = 4096;
    return ZE_RESULT_SUCCESS;
}

ZE_MOCK_SUCCESS(zePhysicalMemCreate, ze_context_handle_t, ze_device_handle_t, ze_physical_mem_desc_t *, ze_physical_mem_handle_t *)
ZE_MOCK_SUCCESS(zePhysicalMemDestroy, ze_context_handle_t, ze_physical_mem_handle_t)
ZE_MOCK_SUCCESS(zeVirtualMemMap, ze_context_handle_t, const void *, size_t, ze_physical_mem_handle_t, size_t, ze_memory_access_attribute_t)
ZE_MOCK_SUCCESS(zeVirtualMemUnmap, ze_context_handle_t, const void *, size_t)
ZE_MOCK_SUCCESS(zeVirtualMemSetAccessAttribute, ze_context_handle_t, const void *, size_t, ze_memory_access_attribute_t)

ZE_APIEXPORT ze_result_t ZE_APICALL zeVirtualMemGetAccessAttribute(
    ze_context_handle_t hContext,
    const void *ptr,
    size_t size,
    ze_memory_access_attribute_t *pAccess) {
    (void)hContext;
    (void)ptr;
    (void)size;
    if (pAccess)
        *pAccess = ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE;
    return ZE_RESULT_SUCCESS;
}

ZE_MOCK_SUCCESS(zeVirtualMemGetAccessAttribute, ze_context_handle_t, const void *, size_t, ze_memory_access_attribute_t *, size_t *)
ZE_MOCK_SUCCESS(zeKernelSetGlobalOffsetExp, ze_kernel_handle_t, uint32_t, uint32_t, uint32_t)
ZE_MOCK_SUCCESS(zeKernelGetBinaryExp, ze_kernel_handle_t, size_t *, uint8_t *)
ZE_MOCK_SUCCESS(zeDeviceImportExternalSemaphoreExt, ze_device_handle_t, const ze_external_semaphore_ext_desc_t *, ze_external_semaphore_ext_handle_t *)
ZE_MOCK_SUCCESS(zeDeviceReleaseExternalSemaphoreExt, ze_external_semaphore_ext_handle_t)
ZE_MOCK_SUCCESS(zeCommandListAppendSignalExternalSemaphoreExt, ze_command_list_handle_t, uint32_t, ze_external_semaphore_ext_handle_t *, ze_external_semaphore_signal_params_ext_t *, ze_event_handle_t, uint32_t, ze_event_handle_t *)
ZE_MOCK_SUCCESS(zeCommandListAppendWaitExternalSemaphoreExt, ze_command_list_handle_t, uint32_t, ze_external_semaphore_ext_handle_t *, ze_external_semaphore_wait_params_ext_t *, ze_event_handle_t, uint32_t, ze_event_handle_t *)
ZE_MOCK_SUCCESS(zeDeviceReserveCacheExt, ze_device_handle_t, size_t, size_t)
ZE_MOCK_SUCCESS(zeDeviceSetCacheAdviceExt, ze_device_handle_t, void *, size_t, ze_cache_ext_region_t)
ZE_MOCK_SUCCESS(zeEventQueryTimestampsExp, ze_event_handle_t, ze_device_handle_t, uint32_t *, ze_kernel_timestamp_result_t *)
ZE_MOCK_SUCCESS(zeImageGetMemoryPropertiesExp, ze_image_handle_t, ze_image_memory_properties_exp_t *)
ZE_MOCK_SUCCESS(zeImageViewCreateExt, ze_context_handle_t, ze_device_handle_t, const ze_image_desc_t *, ze_image_handle_t, ze_image_handle_t *)
ZE_MOCK_SUCCESS(zeImageViewCreateExp, ze_context_handle_t, ze_device_handle_t, const ze_image_desc_t *, ze_image_handle_t, ze_image_handle_t *)
ZE_MOCK_SUCCESS(zeKernelSchedulingHintExp, ze_kernel_handle_t, ze_scheduling_hint_exp_desc_t *)
ZE_MOCK_SUCCESS(zeDevicePciGetPropertiesExt, ze_device_handle_t, ze_pci_ext_properties_t *)
ZE_MOCK_SUCCESS(zeCommandListAppendImageCopyToMemoryExt, ze_command_list_handle_t, void *, ze_image_handle_t, const ze_image_region_t *, uint32_t, uint32_t, ze_event_handle_t, uint32_t, ze_event_handle_t *)
ZE_MOCK_SUCCESS(zeCommandListAppendImageCopyFromMemoryExt, ze_command_list_handle_t, ze_image_handle_t, const void *, const ze_image_region_t *, uint32_t, uint32_t, ze_event_handle_t, uint32_t, ze_event_handle_t *)
ZE_MOCK_SUCCESS(zeImageGetAllocPropertiesExt, ze_context_handle_t, ze_image_handle_t, ze_image_allocation_ext_properties_t *)
ZE_MOCK_SUCCESS(zeModuleInspectLinkageExt, ze_linkage_inspection_ext_desc_t *, uint32_t, ze_module_handle_t *, ze_module_build_log_handle_t *)
ZE_MOCK_SUCCESS(zeMemFreeExt, ze_context_handle_t, const ze_memory_free_ext_desc_t *, void *)
ZE_MOCK_SUCCESS(zeFabricVertexGetExp, ze_driver_handle_t, uint32_t *, ze_fabric_vertex_handle_t *)
ZE_MOCK_SUCCESS(zeFabricVertexGetSubVerticesExp, ze_fabric_vertex_handle_t, uint32_t *, ze_fabric_vertex_handle_t *)
ZE_MOCK_SUCCESS(zeFabricVertexGetPropertiesExp, ze_fabric_vertex_handle_t, ze_fabric_vertex_exp_properties_t *)
ZE_MOCK_SUCCESS(zeFabricVertexGetDeviceExp, ze_fabric_vertex_handle_t, ze_device_handle_t *)
ZE_MOCK_SUCCESS(zeDeviceGetFabricVertexExp, ze_device_handle_t, ze_fabric_vertex_handle_t *)
ZE_MOCK_SUCCESS(zeFabricEdgeGetExp, ze_fabric_vertex_handle_t, ze_fabric_vertex_handle_t, uint32_t *, ze_fabric_edge_handle_t *)
ZE_MOCK_SUCCESS(zeFabricEdgeGetVerticesExp, ze_fabric_edge_handle_t, ze_fabric_vertex_handle_t *, ze_fabric_vertex_handle_t *)
ZE_MOCK_SUCCESS(zeFabricEdgeGetPropertiesExp, ze_fabric_edge_handle_t, ze_fabric_edge_exp_properties_t *)
ZE_MOCK_SUCCESS(zeEventQueryKernelTimestampsExt, ze_event_handle_t, ze_device_handle_t, uint32_t *, ze_event_query_kernel_timestamps_results_ext_properties_t *)
ZE_MOCK_SUCCESS(zeRTASBuilderCreateExp, ze_driver_handle_t, const ze_rtas_builder_exp_desc_t *, ze_rtas_builder_exp_handle_t *)
ZE_MOCK_SUCCESS(zeRTASBuilderGetBuildPropertiesExp, ze_rtas_builder_exp_handle_t, const ze_rtas_builder_build_op_exp_desc_t *, ze_rtas_builder_exp_properties_t *)
ZE_MOCK_SUCCESS(zeDriverRTASFormatCompatibilityCheckExp, ze_driver_handle_t, ze_rtas_format_exp_t, ze_rtas_format_exp_t)
ZE_MOCK_SUCCESS(zeRTASBuilderBuildExp, ze_rtas_builder_exp_handle_t, const ze_rtas_builder_build_op_exp_desc_t *, void *, size_t, void *, size_t, ze_rtas_parallel_operation_exp_handle_t, void *, ze_rtas_aabb_exp_t *, size_t *)
ZE_MOCK_SUCCESS(zeRTASBuilderDestroyExp, ze_rtas_builder_exp_handle_t)
ZE_MOCK_SUCCESS(zeRTASParallelOperationCreateExp, ze_driver_handle_t, ze_rtas_parallel_operation_exp_handle_t *)
ZE_MOCK_SUCCESS(zeRTASParallelOperationGetPropertiesExp, ze_rtas_parallel_operation_exp_handle_t, ze_rtas_parallel_operation_exp_properties_t *)
ZE_MOCK_SUCCESS(zeRTASParallelOperationJoinExp, ze_rtas_parallel_operation_exp_handle_t)
ZE_MOCK_SUCCESS(zeRTASParallelOperationDestroyExp, ze_rtas_parallel_operation_exp_handle_t)
ZE_MOCK_SUCCESS(zeMemGetPitchFor2dImage, ze_context_handle_t, ze_device_handle_t, size_t, size_t, unsigned int, size_t *)
ZE_MOCK_SUCCESS(zeImageGetDeviceOffsetExp, ze_image_handle_t, uint64_t *)
ZE_MOCK_SUCCESS(zeCommandListCreateCloneExp, ze_command_list_handle_t, ze_command_list_handle_t *)
ZE_MOCK_SUCCESS(zeCommandListImmediateAppendCommandListsExp, ze_command_list_handle_t, uint32_t, ze_command_list_handle_t *, ze_event_handle_t, uint32_t, ze_event_handle_t *)
ZE_MOCK_SUCCESS(zeCommandListGetNextCommandIdExp, ze_command_list_handle_t, const ze_mutable_command_id_exp_desc_t *, uint64_t *)
ZE_MOCK_SUCCESS(zeCommandListGetNextCommandIdWithKernelsExp, ze_command_list_handle_t, const ze_mutable_command_id_exp_desc_t *, uint32_t, ze_kernel_handle_t *, uint64_t *)
ZE_MOCK_SUCCESS(zeCommandListUpdateMutableCommandsExp, ze_command_list_handle_t, const ze_mutable_commands_exp_desc_t *)
ZE_MOCK_SUCCESS(zeCommandListUpdateMutableCommandSignalEventExp, ze_command_list_handle_t, uint64_t, ze_event_handle_t)
ZE_MOCK_SUCCESS(zeCommandListUpdateMutableCommandWaitEventsExp, ze_command_list_handle_t, uint64_t, uint32_t, ze_event_handle_t *)
ZE_MOCK_SUCCESS(zeCommandListUpdateMutableCommandKernelsExp, ze_command_list_handle_t, uint32_t, uint64_t *, ze_kernel_handle_t *)

ZE_MOCK_SUCCESS(zesInit, zes_init_flags_t)
ZE_MOCK_SUCCESS(zesDriverGet, uint32_t *, zes_driver_handle_t *)
ZE_MOCK_SUCCESS(zesDriverGetExtensionProperties, zes_driver_handle_t, uint32_t *, zes_driver_extension_properties_t *)
ZE_MOCK_SUCCESS(zesDriverGetExtensionFunctionAddress, zes_driver_handle_t, const char *, void **)
ZE_MOCK_SUCCESS(zesDeviceGet, zes_driver_handle_t, uint32_t *, zes_device_handle_t *)
ZE_MOCK_SUCCESS(zesDeviceGetProperties, zes_device_handle_t, zes_device_properties_t *)
ZE_MOCK_SUCCESS(zesDeviceGetState, zes_device_handle_t, zes_device_state_t *)
ZE_MOCK_SUCCESS(zesDeviceReset, zes_device_handle_t, ze_bool_t)
ZE_MOCK_SUCCESS(zesDeviceResetExt, zes_device_handle_t, zes_reset_properties_t *)
ZE_MOCK_SUCCESS(zesDeviceProcessesGetState, zes_device_handle_t, uint32_t *, zes_process_state_t *)
ZE_MOCK_SUCCESS(zesDevicePciGetProperties, zes_device_handle_t, zes_pci_properties_t *)
ZE_MOCK_SUCCESS(zesDevicePciGetState, zes_device_handle_t, zes_pci_state_t *)
ZE_MOCK_SUCCESS(zesDevicePciGetBars, zes_device_handle_t, uint32_t *, zes_pci_bar_properties_t *)
ZE_MOCK_SUCCESS(zesDevicePciGetStats, zes_device_handle_t, zes_pci_stats_t *)
ZE_MOCK_SUCCESS(zesDeviceSetOverclockWaiver, zes_device_handle_t)
ZE_MOCK_SUCCESS(zesDeviceGetOverclockDomains, zes_device_handle_t, uint32_t *)
ZE_MOCK_SUCCESS(zesDeviceGetOverclockControls, zes_device_handle_t, zes_overclock_domain_t, uint32_t *)
ZE_MOCK_SUCCESS(zesDeviceResetOverclockSettings, zes_device_handle_t, ze_bool_t)
ZE_MOCK_SUCCESS(zesDeviceReadOverclockState, zes_device_handle_t, zes_overclock_mode_t *, ze_bool_t *, ze_bool_t *, zes_pending_action_t *, ze_bool_t *)
ZE_MOCK_SUCCESS(zesDeviceEnumOverclockDomains, zes_device_handle_t, uint32_t *, zes_overclock_handle_t *)
ZE_MOCK_SUCCESS(zesOverclockGetDomainProperties, zes_overclock_handle_t, zes_overclock_properties_t *)
ZE_MOCK_SUCCESS(zesOverclockGetDomainVFProperties, zes_overclock_handle_t, zes_vf_property_t *)
ZE_MOCK_SUCCESS(zesOverclockGetDomainControlProperties, zes_overclock_handle_t, zes_overclock_control_t, zes_control_property_t *)
ZE_MOCK_SUCCESS(zesOverclockGetControlCurrentValue, zes_overclock_handle_t, zes_overclock_control_t, double *)
ZE_MOCK_SUCCESS(zesOverclockGetControlPendingValue, zes_overclock_handle_t, zes_overclock_control_t, double *)
ZE_MOCK_SUCCESS(zesOverclockSetControlUserValue, zes_overclock_handle_t, zes_overclock_control_t, double, zes_pending_action_t *)
ZE_MOCK_SUCCESS(zesOverclockGetControlState, zes_overclock_handle_t, zes_overclock_control_t, zes_control_state_t *, zes_pending_action_t *)
ZE_MOCK_SUCCESS(zesOverclockGetVFPointValues, zes_overclock_handle_t, zes_vf_type_t, zes_vf_array_type_t, uint32_t, uint32_t *)
ZE_MOCK_SUCCESS(zesOverclockSetVFPointValues, zes_overclock_handle_t, zes_vf_type_t, uint32_t, uint32_t)
ZE_MOCK_SUCCESS(zesDeviceEnumDiagnosticTestSuites, zes_device_handle_t, uint32_t *, zes_diag_handle_t *)
ZE_MOCK_SUCCESS(zesDiagnosticsGetProperties, zes_diag_handle_t, zes_diag_properties_t *)
ZE_MOCK_SUCCESS(zesDiagnosticsGetTests, zes_diag_handle_t, uint32_t *, zes_diag_test_t *)
ZE_MOCK_SUCCESS(zesDiagnosticsRunTests, zes_diag_handle_t, uint32_t, uint32_t, zes_diag_result_t *)
ZE_MOCK_SUCCESS(zesDeviceEccAvailable, zes_device_handle_t, ze_bool_t *)
ZE_MOCK_SUCCESS(zesDeviceEccConfigurable, zes_device_handle_t, ze_bool_t *)
ZE_MOCK_SUCCESS(zesDeviceGetEccState, zes_device_handle_t, zes_device_ecc_properties_t *)
ZE_MOCK_SUCCESS(zesDeviceSetEccState, zes_device_handle_t, const zes_device_ecc_desc_t *, zes_device_ecc_properties_t *)
ZE_MOCK_SUCCESS(zesDeviceEnumEngineGroups, zes_device_handle_t, uint32_t *, zes_engine_handle_t *)
ZE_MOCK_SUCCESS(zesEngineGetProperties, zes_engine_handle_t, zes_engine_properties_t *)
ZE_MOCK_SUCCESS(zesEngineGetActivity, zes_engine_handle_t, zes_engine_stats_t *)
ZE_MOCK_SUCCESS(zesDeviceEventRegister, zes_device_handle_t, zes_event_type_flags_t)
ZE_MOCK_SUCCESS(zesDriverEventListen, ze_driver_handle_t, uint32_t, uint32_t, zes_device_handle_t *, uint32_t *, zes_event_type_flags_t *)
ZE_MOCK_SUCCESS(zesDriverEventListenEx, ze_driver_handle_t, uint64_t, uint32_t, zes_device_handle_t *, uint32_t *, zes_event_type_flags_t *)
ZE_MOCK_SUCCESS(zesDeviceEnumFabricPorts, zes_device_handle_t, uint32_t *, zes_fabric_port_handle_t *)
ZE_MOCK_SUCCESS(zesFabricPortGetProperties, zes_fabric_port_handle_t, zes_fabric_port_properties_t *)
ZE_MOCK_SUCCESS(zesFabricPortGetLinkType, zes_fabric_port_handle_t, zes_fabric_link_type_t *)
ZE_MOCK_SUCCESS(zesFabricPortGetConfig, zes_fabric_port_handle_t, zes_fabric_port_config_t *)
ZE_MOCK_SUCCESS(zesFabricPortSetConfig, zes_fabric_port_handle_t, const zes_fabric_port_config_t *)
ZE_MOCK_SUCCESS(zesFabricPortGetState, zes_fabric_port_handle_t, zes_fabric_port_state_t *)
ZE_MOCK_SUCCESS(zesFabricPortGetThroughput, zes_fabric_port_handle_t, zes_fabric_port_throughput_t *)
ZE_MOCK_SUCCESS(zesFabricPortGetFabricErrorCounters, zes_fabric_port_handle_t, zes_fabric_port_error_counters_t *)
ZE_MOCK_SUCCESS(zesFabricPortGetMultiPortThroughput, zes_device_handle_t, uint32_t, zes_fabric_port_handle_t *, zes_fabric_port_throughput_t **)
ZE_MOCK_SUCCESS(zesDeviceEnumFans, zes_device_handle_t, uint32_t *, zes_fan_handle_t *)
ZE_MOCK_SUCCESS(zesFanGetProperties, zes_fan_handle_t, zes_fan_properties_t *)
ZE_MOCK_SUCCESS(zesFanGetConfig, zes_fan_handle_t, zes_fan_config_t *)
ZE_MOCK_SUCCESS(zesFanSetDefaultMode, zes_fan_handle_t)
ZE_MOCK_SUCCESS(zesFanSetFixedSpeedMode, zes_fan_handle_t, const zes_fan_speed_t *)
ZE_MOCK_SUCCESS(zesFanSetSpeedTableMode, zes_fan_handle_t, const zes_fan_speed_table_t *)
ZE_MOCK_SUCCESS(zesFanGetState, zes_fan_handle_t, zes_fan_speed_units_t, int32_t *)
ZE_MOCK_SUCCESS(zesDeviceEnumFirmwares, zes_device_handle_t, uint32_t *, zes_firmware_handle_t *)
ZE_MOCK_SUCCESS(zesFirmwareGetProperties, zes_firmware_handle_t, zes_firmware_properties_t *)
ZE_MOCK_SUCCESS(zesFirmwareFlash, zes_firmware_handle_t, void *, uint32_t)
ZE_MOCK_SUCCESS(zesFirmwareGetFlashProgress, zes_firmware_handle_t, uint32_t *)
ZE_MOCK_SUCCESS(zesFirmwareGetConsoleLogs, zes_firmware_handle_t, size_t *, char *)
ZE_MOCK_SUCCESS(zesDeviceEnumFrequencyDomains, zes_device_handle_t, uint32_t *, zes_freq_handle_t *)
ZE_MOCK_SUCCESS(zesFrequencyGetProperties, zes_freq_handle_t, zes_freq_properties_t *)
ZE_MOCK_SUCCESS(zesFrequencyGetAvailableClocks, zes_freq_handle_t, uint32_t *, double *)
ZE_MOCK_SUCCESS(zesFrequencyGetRange, zes_freq_handle_t, zes_freq_range_t *)
ZE_MOCK_SUCCESS(zesFrequencySetRange, zes_freq_handle_t, const zes_freq_range_t *)
ZE_MOCK_SUCCESS(zesFrequencyGetState, zes_freq_handle_t, zes_freq_state_t *)
ZE_MOCK_SUCCESS(zesFrequencyGetThrottleTime, zes_freq_handle_t, zes_freq_throttle_time_t *)
ZE_MOCK_SUCCESS(zesFrequencyOcGetCapabilities, zes_freq_handle_t, zes_oc_capabilities_t *)
ZE_MOCK_SUCCESS(zesFrequencyOcGetFrequencyTarget, zes_freq_handle_t, double *)
ZE_MOCK_SUCCESS(zesFrequencyOcSetFrequencyTarget, zes_freq_handle_t, double)
ZE_MOCK_SUCCESS(zesFrequencyOcGetVoltageTarget, zes_freq_handle_t, double *, double *)
ZE_MOCK_SUCCESS(zesFrequencyOcSetVoltageTarget, zes_freq_handle_t, double, double)
ZE_MOCK_SUCCESS(zesFrequencyOcSetMode, zes_freq_handle_t, zes_oc_mode_t)
ZE_MOCK_SUCCESS(zesFrequencyOcGetMode, zes_freq_handle_t, zes_oc_mode_t *)
ZE_MOCK_SUCCESS(zesFrequencyOcGetIccMax, zes_freq_handle_t, double *)
ZE_MOCK_SUCCESS(zesFrequencyOcSetIccMax, zes_freq_handle_t, double)
ZE_MOCK_SUCCESS(zesFrequencyOcGetTjMax, zes_freq_handle_t, double *)
ZE_MOCK_SUCCESS(zesFrequencyOcSetTjMax, zes_freq_handle_t, double)
ZE_MOCK_SUCCESS(zesDeviceEnumLeds, zes_device_handle_t, uint32_t *, zes_led_handle_t *)
ZE_MOCK_SUCCESS(zesLedGetProperties, zes_led_handle_t, zes_led_properties_t *)
ZE_MOCK_SUCCESS(zesLedGetState, zes_led_handle_t, zes_led_state_t *)
ZE_MOCK_SUCCESS(zesLedSetState, zes_led_handle_t, ze_bool_t)
ZE_MOCK_SUCCESS(zesLedSetColor, zes_led_handle_t, const zes_led_color_t *)
ZE_MOCK_SUCCESS(zesDeviceEnumMemoryModules, zes_device_handle_t, uint32_t *, zes_mem_handle_t *)
ZE_MOCK_SUCCESS(zesMemoryGetProperties, zes_mem_handle_t, zes_mem_properties_t *)
ZE_MOCK_SUCCESS(zesMemoryGetState, zes_mem_handle_t, zes_mem_state_t *)
ZE_MOCK_SUCCESS(zesMemoryGetBandwidth, zes_mem_handle_t, zes_mem_bandwidth_t *)
ZE_MOCK_SUCCESS(zesDeviceEnumPerformanceFactorDomains, zes_device_handle_t, uint32_t *, zes_perf_handle_t *)
ZE_MOCK_SUCCESS(zesPerformanceFactorGetProperties, zes_perf_handle_t, zes_perf_properties_t *)
ZE_MOCK_SUCCESS(zesPerformanceFactorGetConfig, zes_perf_handle_t, double *)
ZE_MOCK_SUCCESS(zesPerformanceFactorSetConfig, zes_perf_handle_t, double)
ZE_MOCK_SUCCESS(zesDeviceEnumPowerDomains, zes_device_handle_t, uint32_t *, zes_pwr_handle_t *)
ZE_MOCK_SUCCESS(zesDeviceGetCardPowerDomain, zes_device_handle_t, zes_pwr_handle_t *)
ZE_MOCK_SUCCESS(zesPowerGetProperties, zes_pwr_handle_t, zes_power_properties_t *)
ZE_MOCK_SUCCESS(zesPowerGetEnergyCounter, zes_pwr_handle_t, zes_power_energy_counter_t *)
ZE_MOCK_SUCCESS(zesPowerGetLimits, zes_pwr_handle_t, zes_power_sustained_limit_t *, zes_power_burst_limit_t *, zes_power_peak_limit_t *)
ZE_MOCK_SUCCESS(zesPowerSetLimits, zes_pwr_handle_t, const zes_power_sustained_limit_t *, const zes_power_burst_limit_t *, const zes_power_peak_limit_t *)
ZE_MOCK_SUCCESS(zesPowerGetEnergyThreshold, zes_pwr_handle_t, zes_energy_threshold_t *)
ZE_MOCK_SUCCESS(zesPowerSetEnergyThreshold, zes_pwr_handle_t, double)
ZE_MOCK_SUCCESS(zesDeviceEnumPsus, zes_device_handle_t, uint32_t *, zes_psu_handle_t *)
ZE_MOCK_SUCCESS(zesPsuGetProperties, zes_psu_handle_t, zes_psu_properties_t *)
ZE_MOCK_SUCCESS(zesPsuGetState, zes_psu_handle_t, zes_psu_state_t *)
ZE_MOCK_SUCCESS(zesDeviceEnumRasErrorSets, zes_device_handle_t, uint32_t *, zes_ras_handle_t *)
ZE_MOCK_SUCCESS(zesRasGetProperties, zes_ras_handle_t, zes_ras_properties_t *)
ZE_MOCK_SUCCESS(zesRasGetConfig, zes_ras_handle_t, zes_ras_config_t *)
ZE_MOCK_SUCCESS(zesRasSetConfig, zes_ras_handle_t, const zes_ras_config_t *)
ZE_MOCK_SUCCESS(zesRasGetState, zes_ras_handle_t, ze_bool_t, zes_ras_state_t *)
ZE_MOCK_SUCCESS(zesDeviceEnumSchedulers, zes_device_handle_t, uint32_t *, zes_sched_handle_t *)
ZE_MOCK_SUCCESS(zesSchedulerGetProperties, zes_sched_handle_t, zes_sched_properties_t *)
ZE_MOCK_SUCCESS(zesSchedulerGetCurrentMode, zes_sched_handle_t, zes_sched_mode_t *)
ZE_MOCK_SUCCESS(zesSchedulerGetTimeoutModeProperties, zes_sched_handle_t, ze_bool_t, zes_sched_timeout_properties_t *)
ZE_MOCK_SUCCESS(zesSchedulerGetTimesliceModeProperties, zes_sched_handle_t, ze_bool_t, zes_sched_timeslice_properties_t *)
ZE_MOCK_SUCCESS(zesSchedulerSetTimeoutMode, zes_sched_handle_t, zes_sched_timeout_properties_t *, ze_bool_t *)
ZE_MOCK_SUCCESS(zesSchedulerSetTimesliceMode, zes_sched_handle_t, zes_sched_timeslice_properties_t *, ze_bool_t *)
ZE_MOCK_SUCCESS(zesSchedulerSetExclusiveMode, zes_sched_handle_t, ze_bool_t *)
ZE_MOCK_SUCCESS(zesSchedulerSetComputeUnitDebugMode, zes_sched_handle_t, ze_bool_t *)
ZE_MOCK_SUCCESS(zesDeviceEnumStandbyDomains, zes_device_handle_t, uint32_t *, zes_standby_handle_t *)
ZE_MOCK_SUCCESS(zesStandbyGetProperties, zes_standby_handle_t, zes_standby_properties_t *)
ZE_MOCK_SUCCESS(zesStandbyGetMode, zes_standby_handle_t, zes_standby_promo_mode_t *)
ZE_MOCK_SUCCESS(zesStandbySetMode, zes_standby_handle_t, zes_standby_promo_mode_t)
ZE_MOCK_SUCCESS(zesDeviceEnumTemperatureSensors, zes_device_handle_t, uint32_t *, zes_temp_handle_t *)
ZE_MOCK_SUCCESS(zesTemperatureGetProperties, zes_temp_handle_t, zes_temp_properties_t *)
ZE_MOCK_SUCCESS(zesTemperatureGetConfig, zes_temp_handle_t, zes_temp_config_t *)
ZE_MOCK_SUCCESS(zesTemperatureSetConfig, zes_temp_handle_t, const zes_temp_config_t *)
ZE_MOCK_SUCCESS(zesTemperatureGetState, zes_temp_handle_t, double *)
ZE_MOCK_SUCCESS(zesPowerGetLimitsExt, zes_pwr_handle_t, uint32_t *, zes_power_limit_ext_desc_t *)
ZE_MOCK_SUCCESS(zesPowerSetLimitsExt, zes_pwr_handle_t, uint32_t *, zes_power_limit_ext_desc_t *)
ZE_MOCK_SUCCESS(zesEngineGetActivityExt, zes_engine_handle_t, uint32_t *, zes_engine_stats_t *)
ZE_MOCK_SUCCESS(zesRasGetStateExp, zes_ras_handle_t, uint32_t *, zes_ras_state_exp_t *)
ZE_MOCK_SUCCESS(zesRasClearStateExp, zes_ras_handle_t, zes_ras_error_category_exp_t)
ZE_MOCK_SUCCESS(zesFirmwareGetSecurityVersionExp, zes_firmware_handle_t, char *)
ZE_MOCK_SUCCESS(zesFirmwareSetSecurityVersionExp, zes_firmware_handle_t)
ZE_MOCK_SUCCESS(zesDeviceGetSubDevicePropertiesExp, zes_device_handle_t, uint32_t *, zes_subdevice_exp_properties_t *)
ZE_MOCK_SUCCESS(zesDriverGetDeviceByUuidExp, zes_driver_handle_t, zes_uuid_t, zes_device_handle_t *, ze_bool_t *, uint32_t *)
ZE_MOCK_SUCCESS(zesDeviceEnumActiveVFExp, zes_device_handle_t, uint32_t *, zes_vf_handle_t *)
ZE_MOCK_SUCCESS(zesVFManagementGetVFPropertiesExp, zes_vf_handle_t, zes_vf_exp_properties_t *)
ZE_MOCK_SUCCESS(zesVFManagementGetVFMemoryUtilizationExp, zes_vf_handle_t, uint32_t *, zes_vf_util_mem_exp_t *)
ZE_MOCK_SUCCESS(zesVFManagementGetVFEngineUtilizationExp, zes_vf_handle_t, uint32_t *, zes_vf_util_engine_exp_t *)
ZE_MOCK_SUCCESS(zesVFManagementSetVFTelemetryModeExp, zes_vf_handle_t, zes_vf_info_util_exp_flags_t, ze_bool_t)
ZE_MOCK_SUCCESS(zesVFManagementSetVFTelemetrySamplingIntervalExp, zes_vf_handle_t, zes_vf_info_util_exp_flags_t, uint64_t)
ZE_MOCK_SUCCESS(zesDeviceEnumEnabledVFExp, zes_device_handle_t, uint32_t *, zes_vf_handle_t *)
ZE_MOCK_SUCCESS(zesVFManagementGetVFCapabilitiesExp, zes_vf_handle_t, zes_vf_exp_capabilities_t *)
ZE_MOCK_SUCCESS(zesVFManagementGetVFMemoryUtilizationExp2, zes_vf_handle_t, uint32_t *, zes_vf_util_mem_exp2_t *)
ZE_MOCK_SUCCESS(zesVFManagementGetVFEngineUtilizationExp2, zes_vf_handle_t, uint32_t *, zes_vf_util_engine_exp2_t *)
ZE_MOCK_SUCCESS(zesVFManagementGetVFCapabilitiesExp2, zes_vf_handle_t, zes_vf_exp2_capabilities_t *)

// ze_intel_gpu.h
ZE_MOCK_FAILURE(zeIntelGetDriverVersionString, ze_driver_handle_t, char *, size_t *)
ZE_MOCK_FAILURE(zeIntelKernelGetBinaryExp, ze_kernel_handle_t, size_t *, char *)
ZE_MOCK_FAILURE(zeIntelImageGetFormatModifiersSupportedExp, ze_device_handle_t, const ze_image_desc_t *const, uint32_t *, uint64_t *)
ZE_MOCK_FAILURE(zeIntelMemGetFormatModifiersSupportedExp, ze_context_handle_t, const ze_device_mem_alloc_desc_t *const, size_t, size_t, ze_device_handle_t, uint32_t *, uint64_t *)
ZE_MOCK_SUCCESS(zeDeviceGetPriorityLevels, ze_device_handle_t, int *, int *)

ze_context_handle_t zeDriverGetDefaultContext(ze_driver_handle_t hDriver) {
    (void)hDriver;
    return reinterpret_cast<ze_context_handle_t>(0x19);
}

ze_context_handle_t zerGetDefaultContext() {
    return reinterpret_cast<ze_context_handle_t>(0x20);
}

uint32_t zerTranslateDeviceHandleToIdentifier(ze_device_handle_t hDevice) {
    (void)hDevice;
    return 42;
}

ze_device_handle_t zerTranslateIdentifierToDeviceHandle(uint32_t identifier) {
    return reinterpret_cast<ze_device_handle_t>(identifier + 0x1000);
}

ZE_MOCK_SUCCESS(zeDeviceSynchronize, ze_device_handle_t);
ZE_MOCK_SUCCESS(zeCommandListAppendLaunchKernelWithArguments, ze_command_list_handle_t, ze_kernel_handle_t, const ze_group_count_t, const ze_group_size_t, void **, const void *const, ze_event_handle_t, uint32_t, ze_event_handle_t *);

ze_result_t zerGetLastErrorDescription(const char **ppString) {
    static const char *errorString = "mock error string";
    *ppString = errorString;
    return ZE_RESULT_SUCCESS;
}

#endif