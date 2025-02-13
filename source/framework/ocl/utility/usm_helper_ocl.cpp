/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "usm_helper_ocl.h"

#include "framework/utility/aligned_allocator.h"

cl_int UsmHelperOcl::allocate(Opencl &opencl,
                              UsmMemoryPlacement placement,
                              size_t bufferSize,
                              Alloc &outAlloc) {
    outAlloc.placement = placement;
    outAlloc.usm = opencl.getExtensions().queryUsmFunctions();
    if (!opencl.getExtensions().areUsmPointersNotNull(outAlloc.usm)) {
        FATAL_ERROR("USM extension not found");
    }
    outAlloc.context = opencl.context;

    cl_int retVal = CL_SUCCESS;

    switch (placement) {
    case UsmMemoryPlacement::Device:
        outAlloc.ptr = outAlloc.usm.clDeviceMemAllocINTEL(opencl.context, opencl.device, nullptr, bufferSize, 0, &retVal);
        break;
    case UsmMemoryPlacement::Host:
        outAlloc.ptr = outAlloc.usm.clHostMemAllocINTEL(opencl.context, nullptr, bufferSize, 0, &retVal);
        break;
    case UsmMemoryPlacement::Shared:
        outAlloc.ptr = outAlloc.usm.clSharedMemAllocINTEL(opencl.context, opencl.device, nullptr, bufferSize, 0, &retVal);
        break;
    case UsmMemoryPlacement::NonUsmMisaligned:
        outAlloc.ptr = Allocator::allocMisaligned(bufferSize, misalignedOffset);
        break;
    case UsmMemoryPlacement::NonUsm4KBAligned:
        outAlloc.ptr = Allocator::alloc4KBAligned(bufferSize);
        break;
    case UsmMemoryPlacement::NonUsm2MBAligned:
        outAlloc.ptr = Allocator::alloc2MBAligned(bufferSize);
        break;
    case UsmMemoryPlacement::NonUsmMapped:
        outAlloc.mappedData.queue = opencl.commandQueue;
        outAlloc.mappedData.memObject = clCreateBuffer(outAlloc.context, CL_MEM_READ_WRITE, bufferSize, nullptr, &retVal);
        CL_SUCCESS_OR_RETURN(retVal);
        outAlloc.ptr = clEnqueueMapBuffer(outAlloc.mappedData.queue, outAlloc.mappedData.memObject, CL_BLOCKING,
                                          CL_MAP_READ | CL_MAP_WRITE, 0, bufferSize, 0u, nullptr, nullptr, &retVal);
        CL_SUCCESS_OR_RETURN(retVal);
        break;
    default:
        FATAL_ERROR("Unknown placement");
    }

    return retVal;
}

cl_int UsmHelperOcl::deallocate(Alloc &alloc) {

    cl_int retVal = CL_SUCCESS;

    switch (alloc.placement) {
    case UsmMemoryPlacement::Device:
    case UsmMemoryPlacement::Host:
    case UsmMemoryPlacement::Shared:
        retVal = alloc.usm.clMemFreeINTEL(alloc.context, alloc.ptr);
        break;
    case UsmMemoryPlacement::NonUsm4KBAligned:
    case UsmMemoryPlacement::NonUsm2MBAligned:
        Allocator::alignedFree(alloc.ptr);
        break;
    case UsmMemoryPlacement::NonUsmMisaligned:
        Allocator::misalignedFree(alloc.ptr, misalignedOffset);
        break;
    case UsmMemoryPlacement::NonUsmMapped:
        CL_SUCCESS_OR_RETURN(clEnqueueUnmapMemObject(alloc.mappedData.queue, alloc.mappedData.memObject, alloc.ptr, 0, nullptr, nullptr));
        CL_SUCCESS_OR_RETURN(clReleaseMemObject(alloc.mappedData.memObject));
        CL_SUCCESS_OR_RETURN(clFinish(alloc.mappedData.queue));
        break;
    default:
        FATAL_ERROR("Unknown placement");
    }

    alloc = {};

    return retVal;
}

void *UsmHelperOcl::allocate(DeviceSelection placement, Opencl &opencl, size_t bufferSize, cl_int *retVal) {
    const DeviceSelection gpuDevice = DeviceSelectionHelper::withoutHost(placement);
    const auto gpuDevicesCount = DeviceSelectionHelper::getDevicesCount(gpuDevice);
    FATAL_ERROR_IF(gpuDevicesCount > 1, "USM allocations can have 0 or 1 gpu device");

    const bool hasDevice = gpuDevicesCount == 1;
    const bool hasHost = DeviceSelectionHelper::hasDevice(placement, DeviceSelection::Host);

    if (hasDevice && hasHost) {
        auto clSharedMemAllocINTEL = (pfn_clDeviceMemAllocINTEL)clGetExtensionFunctionAddressForPlatform(opencl.platform, "clSharedMemAllocINTEL");
        return clSharedMemAllocINTEL(opencl.context, opencl.getDevice(gpuDevice), nullptr, bufferSize, 0, retVal);
    }

    if (hasDevice) {
        auto clDeviceMemAllocINTEL = (pfn_clDeviceMemAllocINTEL)clGetExtensionFunctionAddressForPlatform(opencl.platform, "clDeviceMemAllocINTEL");
        return clDeviceMemAllocINTEL(opencl.context, opencl.getDevice(gpuDevice), nullptr, bufferSize, 0, retVal);
    }

    if (hasHost) {
        auto clHostMemAllocINTEL = (pfn_clHostMemAllocINTEL)clGetExtensionFunctionAddressForPlatform(opencl.platform, "clHostMemAllocINTEL");
        return clHostMemAllocINTEL(opencl.context, nullptr, bufferSize, 0, retVal);
    }

    FATAL_ERROR("USM allocations need at least one storage location");
}

cl_mem_properties_intel UsmHelperOcl::getInitialPlacementFlag(UsmInitialPlacement placement) {
    switch (placement) {
    case UsmInitialPlacement::Any:
        return 0;
    case UsmInitialPlacement::Host:
        return CL_MEM_ALLOC_INITIAL_PLACEMENT_HOST_INTEL;
    case UsmInitialPlacement::Device:
        return CL_MEM_ALLOC_INITIAL_PLACEMENT_DEVICE_INTEL;
    default:
        FATAL_ERROR("Unknown USM initial placement");
    }
}
