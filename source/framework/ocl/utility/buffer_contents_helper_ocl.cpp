/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "buffer_contents_helper_ocl.h"

#include "framework/ocl/function_signatures_ocl.h"
#include "framework/ocl/utility/error.h"

#include <memory>

cl_int BufferContentsHelperOcl::fillBuffer(cl_command_queue queue, cl_mem buffer, size_t bufferSize, BufferContents contents) {
    switch (contents) {
    case BufferContents::Zeros:
        return fillBufferWithZeros(queue, buffer, bufferSize);
    case BufferContents::Random:
        return fillBufferWithRandomBytes(queue, buffer, bufferSize);
    case BufferContents::IncreasingBytes:
        return fillBufferWithIncreasingBytes(queue, buffer, bufferSize);
    default:
        FATAL_ERROR("Unknown buffer contents");
    }
}

cl_int BufferContentsHelperOcl::fillUsmBufferOrHostPtr(cl_command_queue queue, void *ptr, size_t ptrSize, UsmMemoryPlacement placement, BufferContents contents) {
    switch (placement) {
    case UsmMemoryPlacement::Device:
    case UsmMemoryPlacement::Host:
    case UsmMemoryPlacement::Shared:
        return fillUsmBuffer(queue, ptr, ptrSize, contents);
    case UsmMemoryPlacement::NonUsmMapped:
    case UsmMemoryPlacement::NonUsmMisaligned:
    case UsmMemoryPlacement::NonUsm4KBAligned:
    case UsmMemoryPlacement::NonUsm2MBAligned:
        fill(static_cast<uint8_t *>(ptr), ptrSize, contents);
        return CL_SUCCESS;
    default:
        FATAL_ERROR("Unknown usm memory placement");
    }
}

cl_int BufferContentsHelperOcl::fillUsmBufferOrHostPtr(cl_command_queue queue, void *ptr, size_t ptrSize, HostptrReuseMode reuseMode, BufferContents contents) {
    switch (reuseMode) {
    case HostptrReuseMode::Usm:
        return fillUsmBuffer(queue, ptr, ptrSize, contents);
    case HostptrReuseMode::Aligned4KB:
    case HostptrReuseMode::Misaligned:
    case HostptrReuseMode::Map:
        fill(static_cast<uint8_t *>(ptr), ptrSize, contents);
        return CL_SUCCESS;
    default:
        FATAL_ERROR("Unknown host ptr reuse mode");
    }
}

cl_int BufferContentsHelperOcl::fillBufferWithRandomBytes(cl_command_queue queue, cl_mem buffer, size_t bufferSize) {
    auto cpuBuffer = std::make_unique<uint8_t[]>(bufferSize);
    BufferContentsHelper::fillWithRandomBytes(cpuBuffer.get(), bufferSize);
    CL_SUCCESS_OR_RETURN(clEnqueueWriteBuffer(queue, buffer, CL_BLOCKING, 0, bufferSize, cpuBuffer.get(), 0, nullptr, nullptr));
    return CL_SUCCESS;
}

cl_int BufferContentsHelperOcl::fillBufferWithZeros(cl_command_queue queue, cl_mem buffer, size_t bufferSize) {
    const cl_uint pattern[] = {0};
    CL_SUCCESS_OR_RETURN(clEnqueueFillBuffer(queue, buffer, pattern, sizeof(pattern), 0, bufferSize, 0, nullptr, nullptr));
    CL_SUCCESS_OR_RETURN(clFinish(queue));
    return CL_SUCCESS;
}

cl_int BufferContentsHelperOcl::fillBufferWithIncreasingBytes(cl_command_queue queue, cl_mem buffer, size_t bufferSize) {
    auto cpuBuffer = std::make_unique<uint8_t[]>(bufferSize);
    BufferContentsHelper::fillWithIncreasingBytes(cpuBuffer.get(), bufferSize);
    CL_SUCCESS_OR_RETURN(clEnqueueWriteBuffer(queue, buffer, CL_BLOCKING, 0, bufferSize, cpuBuffer.get(), 0, nullptr, nullptr));
    return CL_SUCCESS;
}

cl_int BufferContentsHelperOcl::fillUsmBuffer(cl_command_queue queue, void *usmBuffer, size_t bufferSize, BufferContents contents) {
    switch (contents) {
    case BufferContents::Zeros:
        return fillUsmBufferWithZeros(queue, usmBuffer, bufferSize);
    case BufferContents::Random:
        return fillUsmBufferWithRandomBytes(queue, usmBuffer, bufferSize);
    default:
        FATAL_ERROR("Unknown buffer contents");
    }
}

cl_int BufferContentsHelperOcl::fillUsmBufferWithRandomBytes(cl_command_queue queue, void *usmBuffer, size_t bufferSize) {
    // Get API calls
    cl_device_id device = {};
    CL_SUCCESS_OR_RETURN(clGetCommandQueueInfo(queue, CL_QUEUE_DEVICE, sizeof(device), &device, nullptr));
    cl_context context = {};
    CL_SUCCESS_OR_RETURN(clGetCommandQueueInfo(queue, CL_QUEUE_CONTEXT, sizeof(context), &context, nullptr));
    cl_platform_id platform = {};
    CL_SUCCESS_OR_RETURN(clGetDeviceInfo(device, CL_DEVICE_PLATFORM, sizeof(platform), &platform, nullptr));
    auto clHostMemAllocINTEL = (pfn_clHostMemAllocINTEL)clGetExtensionFunctionAddressForPlatform(platform, "clHostMemAllocINTEL");
    auto clMemFreeINTEL = (pfn_clMemFreeINTEL)clGetExtensionFunctionAddressForPlatform(platform, "clMemFreeINTEL");
    auto clEnqueueMemcpyINTEL = (pfn_clEnqueueMemcpyINTEL)clGetExtensionFunctionAddressForPlatform(platform, "clEnqueueMemcpyINTEL");

    // Create staging allocation
    cl_int retVal = {};
    void *stagingAlloc = clHostMemAllocINTEL(context, nullptr, bufferSize, 0, &retVal);
    CL_SUCCESS_OR_RETURN(retVal);
    fillWithRandomBytes(static_cast<uint8_t *>(stagingAlloc), bufferSize);

    // Copy to destination allocation
    CL_SUCCESS_OR_RETURN(clEnqueueMemcpyINTEL(queue, CL_NON_BLOCKING, usmBuffer, stagingAlloc, bufferSize, 0, nullptr, nullptr));
    CL_SUCCESS_OR_RETURN(clFinish(queue));
    CL_SUCCESS_OR_RETURN(clMemFreeINTEL(context, stagingAlloc));

    return CL_SUCCESS;
}

cl_int BufferContentsHelperOcl::fillUsmBufferWithZeros(cl_command_queue queue, void *usmBuffer, size_t bufferSize) {
    // Get API calls
    cl_device_id device = {};
    CL_SUCCESS_OR_RETURN(clGetCommandQueueInfo(queue, CL_QUEUE_DEVICE, sizeof(device), &device, nullptr));
    cl_platform_id platform = {};
    CL_SUCCESS_OR_RETURN(clGetDeviceInfo(device, CL_DEVICE_PLATFORM, sizeof(platform), &platform, nullptr));
    auto clEnqueueMemFillINTEL = (pfn_clEnqueueMemFillINTEL)clGetExtensionFunctionAddressForPlatform(platform, "clEnqueueMemFillINTEL");

    // Fill with zeros
    const cl_uint pattern[] = {0};
    CL_SUCCESS_OR_RETURN(clEnqueueMemFillINTEL(queue, usmBuffer, pattern, sizeof(pattern), bufferSize, 0, nullptr, nullptr));
    CL_SUCCESS_OR_RETURN(clFinish(queue));

    return CL_SUCCESS;
}
