/*
 * Copyright (C) 2022-2025 Intel Corporation
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
#include "framework/utility/file_helper.h"
#include "framework/utility/memory_constants.h"
#include "framework/utility/timer.h"

#include "definitions/slm.h"

#include <gtest/gtest.h>
#include <numeric>

static TestResult run(const SlmTrafficArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Latency, MeasurementType::Gpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    if (arguments.writeDirection == true) {
        return TestResult::DeviceNotCapable;
    }

    // Setup
    cl_int retVal;
    QueueProperties queueProperties = QueueProperties::create().setProfiling(true);
    Opencl opencl(queueProperties);

    // Check platform we're on
    IntelProduct intelProduct = getIntelProduct(opencl);
    IntelGen gpuGen = getIntelGen(intelProduct);

    const cl_uint numThreadsPerEu = (gpuGen >= IntelGen::XeHpCore) ? 8 : 7;

    const size_t numOfLoops = 128U;
    const size_t lws = 256;
    const size_t vectorSize = 4;
    const bool printPerEnqueueStats = false;
    const bool buildFromFile = false;
    size_t euNum = 0;
    size_t gpuFreq = 0;

    ASSERT_CL_SUCCESS(clGetDeviceInfo(opencl.device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(euNum), &euNum, nullptr));
    ASSERT_CL_SUCCESS(clGetDeviceInfo(opencl.device, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(gpuFreq), &gpuFreq, nullptr));

    const size_t workSize = euNum * numThreadsPerEu * 16;
    const size_t inOutBuffSize = workSize * vectorSize * sizeof(uint32_t);
    const size_t numDataGroups = 5;

    CompilerOptionsBuilder buildOptions{};
    buildOptions.addOption("-cl-no-signed-zeros ");
    buildOptions.addOption("-cl-std=CL3.0 ");
    buildOptions.addOption("-cl-mad-enable ");
    buildOptions.addOption("-cl-fast-relaxed-math ");
    buildOptions.addDefinitionKeyValue("KERNEL_LOOP_ITERATIONS", numOfLoops);
    buildOptions.addDefinitionKeyValue("MAX_GROUPS", numDataGroups);

    // Create buffer
    const cl_mem_flags memFlagsIn = CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR;
    const cl_mem_flags memFlagsOut = CL_MEM_READ_WRITE;

    auto hostInBuffer = std::make_unique<int[]>(inOutBuffSize / sizeof(int));
    int *pBuff = hostInBuffer.get();
    for (size_t i = 0; i < inOutBuffSize / sizeof(int); i++) {
        *(pBuff++) = (int)i;
    }
    const cl_mem source = clCreateBuffer(opencl.context, memFlagsIn, inOutBuffSize, hostInBuffer.get(), &retVal);
    ASSERT_CL_SUCCESS(retVal);

    const cl_mem destination = clCreateBuffer(opencl.context, memFlagsOut, inOutBuffSize + vectorSize, nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);

    const size_t sharedDataSize = 256UL;

    const size_t slmInitData = (size_t)(sharedDataSize * numDataGroups * sizeof(float));
    auto hostBuffTable = std::make_unique<int[]>(slmInitData);
    int *pBuffI = hostBuffTable.get();
    for (size_t i = 0; i < sharedDataSize; i++) {
        *(pBuffI++) = (int)i;
    }

    const cl_mem slmInitBuff = clCreateBuffer(opencl.context, memFlagsIn, slmInitData, hostBuffTable.get(), &retVal);
    ASSERT_CL_SUCCESS(retVal);

    const char *programSlmRead = NULL;
    size_t programSize = 0;

    if (!buildFromFile) {
        programSlmRead =
            "\n#define LOCAL_SIZE      256"
            "\n#define SLM_MAX_SIZE        (LOCAL_SIZE*MAX_GROUPS)"
            "\n#ifndef INTEL_INTERNAL_DEBUG_H_INCLUDED"
            "\n#define INTEL_INTERNAL_DEBUG_H_INCLUDED"
            "\nulong __attribute__((overloadable)) intel_get_cycle_counter( void );"
            "\n#endif"
            "\n"
            "\ninline uint4 loadSlmSharedData(__local uint* pSharedData, const uint4 x, const uint y) {"
            "\n    const uint  offset = y * LOCAL_SIZE;"
            "\n    const uint4 i = x + offset;"
            "\n    return (uint4)(pSharedData[i.x], pSharedData[i.y], pSharedData[i.z], pSharedData[i.w]);"
            "\n}"
            "\n"
            "\n__attribute__((intel_reqd_sub_group_size(16)))"
            "\n__kernel void slmRead_latencyTest(__global const uint4* restrict src, __global uint4* restrict dst,"
            "\n    __global const uint* restrict pSharedData)"
            "\n{"
            "\n    const size_t x = get_global_id(0);"
            "\n    __local uint  localBuffer[SLM_MAX_SIZE];"
            "\n    uint4 perfData = (uint4)(0, (uint)-1, 0, 0);  //avg, min, max, isOverflow"
            "\n    const uint lid = get_local_id(0);"
            "\n    if (lid < LOCAL_SIZE) {"
            "\n        const uint slmBuffTile = min((uint)get_local_size(0), (uint)LOCAL_SIZE);"
            "\n        const uint slmTiles = SLM_MAX_SIZE / slmBuffTile;"
            "\n        for (uint i=0; i<slmTiles; i++) {"
            "\n            const uint slmIdx = lid + (i*slmBuffTile);"
            "\n            localBuffer[slmIdx] = pSharedData[slmIdx];"
            "\n        }"
            "\n    }"
            "\n    barrier(CLK_LOCAL_MEM_FENCE);"
            "\n    uint4 data = (uint4)(0,0,0,0);"
            "\n    uint nLoop = KERNEL_LOOP_ITERATIONS;"
            "\n    ulong timerON = 0;"
            "\n    ulong timerOFF = 0, time = 0;"
            "\n    for(uint j = 0; j < nLoop; j++) {"
            "\n        timerON = intel_get_cycle_counter();"
            "\n        for (uint i = 1; i < nLoop; i++) {"
            "\n            data += loadSlmSharedData(localBuffer,data.xyzw, 0);"
            "\n            data ^= loadSlmSharedData(localBuffer,data.yzwx, 1);"
            "\n            data ^= loadSlmSharedData(localBuffer,data.zwxy, 2);"
            "\n            data ^= loadSlmSharedData(localBuffer,data.wxyz, 3);"
            "\n        }"
            "\n        timerOFF = intel_get_cycle_counter();"
            "\n        time = ((timerOFF - timerON))/(nLoop*4);"
            "\n        if( ((time >> 32) & 0xFFFFFFFF) > 0 ) {"
            "\n            time = 0xFFFFFFFF;"
            "\n            perfData[3] = 1U;  //overflow"
            "\n        }"
            "\n        perfData[0] = ((perfData[0] * j) + (uint)time)/(j+1);     //cumulative average"
            "\n        perfData[1] = min(perfData[1], (uint)time);               //min"
            "\n        perfData[2] = max(perfData[2], (uint)time);               //max"
            "\n    }"
            "\n    dst[0] = data;"
            "\n    dst[x + 1] = perfData;"
            "\n}";
        programSize = strlen(programSlmRead);
    } else {
        const auto programSrc = FileHelper::loadBinaryFile("slmRead_CryptKernelBased.cl");
        if (programSrc.size() == 0) {
            return TestResult::KernelNotFound;
        }

        programSlmRead = reinterpret_cast<const char *>(programSrc.data());
        programSize = programSrc.size();
    }

    cl_program program{};
    if (TestResult result = ProgramHelperOcl::buildProgramFromSource(opencl.context, opencl.device, programSlmRead, programSize, buildOptions.str().c_str(), program); result != TestResult::Success) {
        if (result != TestResult::Success) {
            size_t numBytes = 0;
            retVal |= clGetProgramBuildInfo(program, opencl.device, CL_PROGRAM_BUILD_LOG, 0, NULL, &numBytes);
            auto buffer = std::make_unique<char[]>(numBytes);
            retVal |= clGetProgramBuildInfo(program, opencl.device, CL_PROGRAM_BUILD_LOG, numBytes, buffer.get(), &numBytes);
            std::cout << buffer.get() << std::endl;
        }

        return TestResult::KernelBuildError;
    }
    cl_kernel kernel = clCreateKernel(program, "slmRead_latencyTest", &retVal);
    ASSERT_CL_SUCCESS(retVal);

    size_t gws = workSize / arguments.occupancyDivider;

    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 0, sizeof(source), &source));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 1, sizeof(destination), &destination));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 2, sizeof(slmInitBuff), &slmInitBuff));
    ASSERT_CL_SUCCESS(retVal);

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        cl_event evt;

        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &gws, &lws, 0, nullptr, &evt));
        ASSERT_CL_SUCCESS(clWaitForEvents(1, &evt));

        const size_t dataReadSize = gws * vectorSize * sizeof(uint32_t);
        auto slmClocksBuffer = std::make_unique<uint32_t[]>(dataReadSize);

        retVal |= clEnqueueReadBuffer(opencl.commandQueue, destination, true, vectorSize * sizeof(uint32_t), dataReadSize, slmClocksBuffer.get(), 0, nullptr, nullptr);
        retVal |= clFinish(opencl.commandQueue);
        ASSERT_CL_SUCCESS(retVal);

        std::vector<size_t> vecPerThreadAverages;
        std::vector<size_t> vecPerThreadMins;
        std::vector<size_t> vecPerThreadMax;
        bool printToConsole = false;
        for (size_t item = 0; item < gws; item++) {
            size_t avgInThread = static_cast<size_t>(slmClocksBuffer.get()[(item * vectorSize)]);
            size_t minInThread = static_cast<size_t>(slmClocksBuffer.get()[(item * vectorSize) + 1]);
            size_t maxInThread = static_cast<size_t>(slmClocksBuffer.get()[(item * vectorSize) + 2]);
            bool isOverflow = (bool)slmClocksBuffer.get()[(item * vectorSize) + 3];
            if (isOverflow) {
                if (printToConsole == false) {
                    std::cout << "BW overflow happens in gws " << gws << std::endl;
                    printToConsole = true;
                }
                continue;
            }
            vecPerThreadAverages.push_back(avgInThread);
            vecPerThreadMins.push_back(minInThread);
            vecPerThreadMax.push_back(maxInThread);
        }

        std::chrono::steady_clock::duration averageLatency = static_cast<std::chrono::steady_clock::duration>(std::accumulate(vecPerThreadAverages.begin(), vecPerThreadAverages.end(), (size_t)0) / vecPerThreadAverages.size());

        if (printPerEnqueueStats) {
            size_t minInAllThreads = *std::min_element(vecPerThreadMins.begin(), vecPerThreadMins.end());
            size_t maxInAllThreads = *std::max_element(vecPerThreadMax.begin(), vecPerThreadMax.end());
            std::cout << "[" << i << "] Average Latency " << static_cast<size_t>(std::accumulate(vecPerThreadAverages.begin(), vecPerThreadAverages.end(), (size_t)0) / vecPerThreadAverages.size()) << " [clk]  min: " << minInAllThreads << " [clk]  max: " << maxInAllThreads << " [clk] (SLM uint4 load.slm.d32)" << std::endl;
        }
        statistics.pushValue(averageLatency, typeSelector.getUnit(), typeSelector.getType());

        cl_ulong timeNs{};
        ASSERT_CL_SUCCESS(ProfilingHelper::getEventDurationInNanoseconds(evt, timeNs));
        ASSERT_CL_SUCCESS(clReleaseEvent(evt));
    }

    // Cleanup
    ASSERT_CL_SUCCESS(clReleaseKernel(kernel));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));
    ASSERT_CL_SUCCESS(clReleaseMemObject(destination));
    ASSERT_CL_SUCCESS(clReleaseMemObject(source));
    ASSERT_CL_SUCCESS(clReleaseMemObject(slmInitBuff));

    return TestResult::Success;
}

static RegisterTestCaseImplementation<SlmTraffic> registerTestCase(run, Api::OpenCL);
