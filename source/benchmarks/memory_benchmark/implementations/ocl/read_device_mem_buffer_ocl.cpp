/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/intel_product/get_intel_product_ocl.h"
#include "framework/ocl/opencl.h"
#include "framework/ocl/utility/compression_helper.h"
#include "framework/ocl/utility/profiling_helper.h"
#include "framework/ocl/utility/program_helper_ocl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/compiler_options_builder.h"
#include "framework/utility/memory_constants.h"
#include "framework/utility/timer.h"

#include "definitions/read_device_mem_buffer.h"

#include <gtest/gtest.h>
using namespace MemoryConstants;

static TestResult run(const ReadDeviceMemBufferArguments &arguments, Statistics &statistics) {
    if (arguments.compressed && arguments.noIntelExtensions) {
        return TestResult::DeviceNotCapable;
    }

    // Setup
    cl_int retVal;
    QueueProperties queueProperties = QueueProperties::create().setProfiling(true);
    Opencl opencl(queueProperties);

    // Check platform we're on
    IntelProduct intelProduct = getIntelProduct(opencl);
    IntelGen gpuGen = getIntelGen(intelProduct);
    if (gpuGen == IntelGen::Unknown) {
        return TestResult::NoImplementation;
    }

    const size_t subgroupSize = 16;
    const size_t vectorSize = 4;
    const size_t singleSendSizeInBytes = subgroupSize * vectorSize * sizeof(float);
    const size_t numOfSends = 16U; // per 2k-4k tiles in one loop iteration
    const size_t numOfLoops = 500U;
    const auto threadTileSizeInSubgroup = singleSendSizeInBytes * numOfSends;
    size_t euNum = 0;

    ASSERT_CL_SUCCESS(clGetDeviceInfo(opencl.device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(euNum), &euNum, nullptr));

    const bool useLargeGRF = gpuGen > IntelGen::Gen12lp ? true : false;
    const std::string largeGrfOpt = useLargeGRF ? " -cl-intel-256-GRF-per-thread " : " ";
    const std::string buildOptions = std::string("-cl-std=CL2.0 -cl-mad-enable -cl-fast-relaxed-math ") +
                                     " -D NUM_SENDS=" + std::to_string(numOfSends) +
                                     " -D KERNEL_LOOP_ITERATIONS=" + std::to_string(numOfLoops) +
                                     " -D THREAD_TILE_SIZE=" + std::to_string(threadTileSizeInSubgroup) +
                                     " -D SUBGROUP_SIZE=" + std::to_string(subgroupSize) +
                                     " -D VECTOR_SIZE=" + std::to_string(vectorSize) +
                                     largeGrfOpt +
                                     std::string(" ");

    // Create buffer
    const cl_mem_flags compressionHint = CompressionHelper::getCompressionFlags(arguments.compressed, arguments.noIntelExtensions);
    const cl_mem_flags memFlags = CL_MEM_READ_WRITE | compressionHint;

    auto srcCpuBuffer = std::make_unique<float[]>(arguments.size / sizeof(float));
    float *pBuff = srcCpuBuffer.get();
    float floatVal1 = 0.0f;
    for (size_t i = 0; i < arguments.size / sizeof(float);) {
        for (size_t j = 0; j < 8; j++) {
            *(pBuff++) = floatVal1 + (float)j;
        }
        floatVal1 += 10.0f;
        i += 8;
    }
    const cl_mem source = clCreateBuffer(opencl.context, memFlags | CL_MEM_COPY_HOST_PTR, arguments.size, srcCpuBuffer.get(), &retVal);

    const cl_mem destination = clCreateBuffer(opencl.context, memFlags, arguments.size, nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);

    const uint32_t clearGpuBuffSize = 64 * megaByte;
    const cl_mem clearGpuBuff = clCreateBuffer(opencl.context, memFlags, clearGpuBuffSize, nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);

    // Check buffers compression
    auto compressionStatus = CompressionHelper::verifyCompression(source, arguments.compressed, arguments.noIntelExtensions);
    if (compressionStatus != TestResult::Success) {
        ASSERT_CL_SUCCESS(clReleaseMemObject(source));
        return compressionStatus;
    }
    compressionStatus = CompressionHelper::verifyCompression(destination, arguments.compressed, arguments.noIntelExtensions);
    if (compressionStatus != TestResult::Success) {
        ASSERT_CL_SUCCESS(clReleaseMemObject(source));
        ASSERT_CL_SUCCESS(clReleaseMemObject(destination));
        return compressionStatus;
    }

    const char *programSrc =
        "__kernel void ClearCaches(__global unsigned int *pBuffer, unsigned int buffSize) {"
        "   const uint gid = get_global_id(0);"
        "   pBuffer[gid] += 0xDEADBEAF;"
        "}"
        "\n#define SEND_SIZE (VECTOR_SIZE * sizeof(float))"
        "\n__attribute__((intel_reqd_sub_group_size(SUBGROUP_SIZE)))"
        "\n__kernel void ReadOnly(__global float4 *const pSrcBuffer, __global float4 *pDstBuffer,"
        "        unsigned int sliceMask, unsigned int sliceSize, unsigned int slotMask) {"
        "   const uint gid = get_global_id(0);"
        "   const uint sid = ((get_group_id(0) * get_num_sub_groups()) + get_sub_group_id()) % slotMask;"
        "   uint startOffset = 0;"
        "   float4 _out_data = {0.0f, 0.0f, 0.0f, 0.0f};"
        "   float4 _input_data; uint j = 0, i = 0;"
        "   for (j = 0; j < KERNEL_LOOP_ITERATIONS; j++) {"
        "       i = 0; uint slotOffset = sliceSize * (j & sliceMask);"
        "       startOffset = (((THREAD_TILE_SIZE * sid) + slotOffset) / SEND_SIZE);"
        "       __attribute__((opencl_unroll_hint(NUM_SENDS)))"
        "       do {"
        "           _input_data = as_float4(intel_sub_group_block_read4((__global const uint *)((__global uint *const)(&pSrcBuffer[startOffset]))));"
        "           _out_data += _input_data; startOffset += SUBGROUP_SIZE;"
        "       } while (++i < NUM_SENDS);"
        "   }"
        "   if (_out_data.x < 0.0f) {"
        "       pDstBuffer[gid] = _out_data;"
        "   }"
        "}";

    // Create kernel
    const auto programSrcLen = strlen(programSrc);
    cl_program program{};
    if (TestResult result = ProgramHelperOcl::buildProgramFromSource(opencl.context, opencl.device, programSrc, programSrcLen, buildOptions.c_str(), program); result != TestResult::Success) {
        return result;
    }

    cl_kernel kernel = clCreateKernel(program, "ReadOnly", &retVal);
    ASSERT_CL_SUCCESS(retVal);
    cl_kernel clearCacheKernel = clCreateKernel(program, "ClearCaches", &retVal);
    ASSERT_CL_SUCCESS(retVal);

    // Clear L3$ Cache kernel
    const size_t clearGws = clearGpuBuffSize / sizeof(cl_uint);
    const size_t buffSizeInInts = clearGpuBuffSize / sizeof(cl_uint);
    ASSERT_CL_SUCCESS(clSetKernelArg(clearCacheKernel, 0, sizeof(clearGpuBuff), &clearGpuBuff));
    ASSERT_CL_SUCCESS(clSetKernelArg(clearCacheKernel, 1, sizeof(buffSizeInInts), &buffSizeInInts));
    ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, clearCacheKernel, 1, nullptr, &clearGws, NULL, 0, nullptr, nullptr));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
    ASSERT_CL_SUCCESS(retVal);

    cl_uint sliceSize = 0, sliceMask = 1, slotMask = 1;
    const cl_uint numThreadsPerEu = (gpuGen >= IntelGen::XeHpCore) ? 8 : 7;
    const cl_uint numHwThreads = (useLargeGRF ? numThreadsPerEu / 2 : 7) * static_cast<cl_uint>(euNum);

    const size_t lws = subgroupSize * 2;
    size_t gws = numHwThreads * subgroupSize;

    // check if surface can be covered with more than one slice
    if ((2 * numHwThreads * threadTileSizeInSubgroup) < static_cast<cl_uint>(arguments.size)) {
        slotMask = numHwThreads;
        sliceSize = numHwThreads * threadTileSizeInSubgroup;
        const auto numMaxSlices = static_cast<cl_uint>(arguments.size) / sliceSize;
        // can cover with more than one slice for all threads
        sliceMask = 1;
        do {
            sliceMask *= 2;
        } while (sliceMask <= numMaxSlices);

        sliceMask /= 2;
        sliceMask -= 1;
    } else if (arguments.size > 256 * kiloByte && arguments.size <= 32 * megaByte) {
        // hit cache
        sliceSize = 256 * kiloByte;
        sliceMask = (static_cast<cl_uint>(arguments.size) / sliceSize) - 1;
        slotMask = sliceSize / threadTileSizeInSubgroup;
    } else {
        slotMask = static_cast<cl_uint>(arguments.size) / threadTileSizeInSubgroup;
    }

    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 0, sizeof(source), &source));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 1, sizeof(destination), &destination));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 2, sizeof(sliceMask), &sliceMask));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 3, sizeof(sliceSize), &sliceSize));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 4, sizeof(slotMask), &slotMask));
    ASSERT_CL_SUCCESS(retVal);

    // warmup
    ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &gws, &lws, 0, nullptr, nullptr));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
    ASSERT_CL_SUCCESS(retVal);

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        cl_event evt;

        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, clearCacheKernel, 1, nullptr, &clearGws, NULL, 0, nullptr, nullptr));
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));

        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &gws, &lws, 0, nullptr, &evt));
        ASSERT_CL_SUCCESS(clWaitForEvents(1, &evt));

        cl_ulong timeNs{};
        ASSERT_CL_SUCCESS(ProfilingHelper::getEventDurationInNanoseconds(evt, timeNs));
        const size_t totalAccessedMemory = (numHwThreads * threadTileSizeInSubgroup * numOfLoops);
        statistics.pushValue(std::chrono::nanoseconds(timeNs), totalAccessedMemory, MeasurementUnit::GigabytesPerSecond, MeasurementType::Gpu);
        ASSERT_CL_SUCCESS(clReleaseEvent(evt));
    }

    // Cleanup
    ASSERT_CL_SUCCESS(clReleaseKernel(kernel));
    ASSERT_CL_SUCCESS(clReleaseKernel(clearCacheKernel));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));
    ASSERT_CL_SUCCESS(clReleaseMemObject(destination));
    ASSERT_CL_SUCCESS(clReleaseMemObject(source));
    ASSERT_CL_SUCCESS(clReleaseMemObject(clearGpuBuff));

    return TestResult::Success;
}

static RegisterTestCaseImplementation<ReadDeviceMemBuffer> registerTestCase(run, Api::OpenCL);
