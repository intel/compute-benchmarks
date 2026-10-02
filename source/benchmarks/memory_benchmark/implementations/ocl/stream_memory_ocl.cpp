/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/ocl/utility/buffer_contents_helper_ocl.h"
#include "framework/ocl/utility/profiling_helper.h"
#include "framework/ocl/utility/program_helper_ocl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/compiler_options_builder.h"
#include "framework/utility/memory_constants.h"
#include "framework/utility/timer.h"

#include "definitions/stream_memory.h"
#include "stream_memory_verification.h"

#include <algorithm>
#include <gtest/gtest.h>
#include <vector>

using namespace MemoryConstants;

static TestResult run(const StreamMemoryArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::GigabytesPerSecond, arguments.useEvents ? MeasurementType::Gpu : MeasurementType::Cpu);

    if (arguments.partialMultiplier > 1u) {
        if (arguments.type == StreamMemoryType::Scale || arguments.type == StreamMemoryType::Triad) {
            return TestResult::NoImplementation;
        }
        if (arguments.type == StreamMemoryType::Write && arguments.contents != BufferContents::Zeros) {
            return TestResult::NoImplementation;
        }
    }

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    cl_int retVal = {};
    QueueProperties queueProperties = QueueProperties::create().setProfiling(true).setOoq(0);
    Opencl opencl(queueProperties);
    Timer timer;

    // Query max workgroup size
    size_t maxWorkgroupSize = {};
    clGetDeviceInfo(opencl.device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(maxWorkgroupSize), &maxWorkgroupSize, nullptr);
    if (arguments.lws > maxWorkgroupSize) {
        return TestResult::DeviceNotCapable;
    }

    size_t elementSize = arguments.vectorSize * sizeof(uint32_t);
    unsigned int scalarValue[16] = {2u, 2u, 2u, 2u, 2u, 2u, 2u, 2u, 2u, 2u, 2u, 2u, 2u, 2u, 2u, 2u};
    bool setScalarArgument = true;
    const bool printBuildInfo = true;

    // Create kernel-specific buffers
    const char *kernelName = {};
    const size_t localWorkSize = std::min<size_t>(arguments.lws, arguments.size / elementSize);
    if (localWorkSize == 0) {
        return TestResult::InvalidArgs;
    }
    const size_t globalWorkSize = arguments.size / elementSize / localWorkSize * localWorkSize;
    size_t bufferSize = globalWorkSize * elementSize;
    cl_mem buffers[3] = {};
    size_t buffersCount = {};
    size_t bufferSizes[3] = {bufferSize, bufferSize, bufferSize};

    auto bufferFlags = CL_MEM_READ_WRITE;
    if (arguments.memoryPlacement == UsmRuntimeMemoryPlacement::Host) {
        bufferFlags |= CL_MEM_FORCE_HOST_MEMORY_INTEL;
    }

    switch (arguments.type) {
    case StreamMemoryType::Read:
        kernelName = "readWithMultiplier";
        buffers[buffersCount++] = clCreateBuffer(opencl.context, bufferFlags, bufferSize, nullptr, &retVal);
        bufferSizes[buffersCount] = 16u;
        buffers[buffersCount++] = clCreateBuffer(opencl.context, bufferFlags, 16u, nullptr, &retVal);
        break;
    case StreamMemoryType::Write:
        if (BufferContents::Random == arguments.contents) {
            kernelName = "write_random";
            setScalarArgument = false; // value to write is embedded in kernel code
        } else {
            kernelName = "writeWithMultiplier";
        }
        buffers[buffersCount++] = clCreateBuffer(opencl.context, bufferFlags, bufferSize, nullptr, &retVal);
        break;
    case StreamMemoryType::Scale:
        kernelName = "scale";
        buffers[buffersCount++] = clCreateBuffer(opencl.context, bufferFlags, bufferSize, nullptr, &retVal);
        buffers[buffersCount++] = clCreateBuffer(opencl.context, bufferFlags, bufferSize, nullptr, &retVal);
        break;
    case StreamMemoryType::Triad:
        kernelName = "triad";
        buffers[buffersCount++] = clCreateBuffer(opencl.context, bufferFlags, bufferSize, nullptr, &retVal);
        buffers[buffersCount++] = clCreateBuffer(opencl.context, bufferFlags, bufferSize, nullptr, &retVal);
        buffers[buffersCount++] = clCreateBuffer(opencl.context, bufferFlags, bufferSize, nullptr, &retVal);
        break;
    default:
        FATAL_ERROR("Unknown StreamMemoryType");
    }

    // Create kernel
    CompilerOptionsBuilder compilerOptions;
    std::string streamType = "uint";
    if (arguments.vectorSize > 1) {
        streamType += std::to_string(arguments.vectorSize);
    }
    compilerOptions.addDefinitionKeyValue("STREAM_TYPE", streamType.c_str());
    compilerOptions.addDefinitionKeyValue("WRITE_RANDOM_PATTERN", StreamMemoryVerification::getWriteRandomPatternDefinition());
    const char *programName = "memory_benchmark_stream_memory.cl";
    cl_program program{};
    if (auto result = ProgramHelperOcl::buildProgramFromSourceFile(opencl.context, opencl.device, programName, compilerOptions.str().c_str(), program); result != TestResult::Success) {
        if (result != TestResult::Success && printBuildInfo) {
            size_t numBytes = 0;
            retVal |= clGetProgramBuildInfo(program, opencl.device, CL_PROGRAM_BUILD_LOG, 0, NULL, &numBytes);
            auto buffer = std::make_unique<char[]>(numBytes);
            retVal |= clGetProgramBuildInfo(program, opencl.device, CL_PROGRAM_BUILD_LOG, numBytes, buffer.get(), &numBytes);
            std::cout << buffer.get() << std::endl;
        }
        return result;
    }
    cl_kernel kernel = clCreateKernel(program, kernelName, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    const bool verify = Configuration::get().verify;
    const size_t outputIndex = arguments.type == StreamMemoryType::Read ? 0u : buffersCount - 1;
    const bool poisonOutput = verify && (arguments.type == StreamMemoryType::Scale || arguments.type == StreamMemoryType::Triad);
    for (auto i = 0u; i < buffersCount; i++) {
        if (poisonOutput && i == outputIndex) {
            ASSERT_CL_SUCCESS(clEnqueueFillBuffer(opencl.commandQueue, buffers[i], &StreamMemoryVerification::outputPoison, 1u, 0, bufferSizes[i], 0, nullptr, nullptr));
        } else if (verify && arguments.type == StreamMemoryType::Triad && i == 1 && arguments.contents == BufferContents::Random) {
            const auto y = StreamMemoryVerification::getInput(arguments.contents, bufferSizes[i], 1);
            std::vector<uint8_t> chunk(std::min(bufferSizes[i], VerificationHelper::chunkSize));
            for (size_t offset = 0; offset < bufferSizes[i]; offset += chunk.size()) {
                const size_t length = std::min(chunk.size(), bufferSizes[i] - offset);
                y.copy(chunk.data(), offset, length);
                ASSERT_CL_SUCCESS(clEnqueueWriteBuffer(opencl.commandQueue, buffers[i], CL_BLOCKING, offset, length, chunk.data(), 0, nullptr, nullptr));
            }
        } else {
            ASSERT_CL_SUCCESS(BufferContentsHelperOcl::fillBuffer(opencl.commandQueue, buffers[i], bufferSizes[i], arguments.contents));
        }
        ASSERT_CL_SUCCESS(clSetKernelArg(kernel, static_cast<cl_uint>(i), sizeof(buffers[i]), &buffers[i]))
    }
    if (setScalarArgument) {
        ASSERT_CL_SUCCESS(clSetKernelArg(kernel, static_cast<cl_uint>(buffersCount), elementSize, &scalarValue));
    }

    int multiplier = static_cast<int>(arguments.partialMultiplier);
    if ((arguments.type == StreamMemoryType::Write && arguments.contents != BufferContents::Random) || arguments.type == StreamMemoryType::Read) {
        ASSERT_CL_SUCCESS(clSetKernelArg(kernel, static_cast<cl_uint>(setScalarArgument ? buffersCount + 1 : buffersCount), 4u, &multiplier));
    }

    // Warm up
    ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &globalWorkSize, &localWorkSize, 0, nullptr, nullptr));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));

    std::vector<uint8_t> outputChunk(verify ? std::min(bufferSize, VerificationHelper::chunkSize) : 0u);
    const auto readChunk = [&](size_t offset, size_t size) -> const uint8_t * {
        CL_SUCCESS_OR_RETURN_VALUE(clEnqueueReadBuffer(opencl.commandQueue, buffers[outputIndex], CL_BLOCKING, offset, size, outputChunk.data(), 0, nullptr, nullptr), nullptr);
        return outputChunk.data();
    };
    const auto expected = StreamMemoryVerification::getExpected(arguments.type, arguments.contents, bufferSize, elementSize, arguments.partialMultiplier, scalarValue[0]);
    TestResult result = TestResult::Success;

    for (auto i = 0u; i < arguments.iterations; i++) {
        cl_event profilingEvent{};
        cl_event *eventForEnqueue = arguments.useEvents ? &profilingEvent : nullptr;

        timer.measureStart();
        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &globalWorkSize, &localWorkSize, 0, nullptr, eventForEnqueue));
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
        timer.measureEnd();

        size_t transferSize = bufferSize;
        switch (arguments.type) {
        case StreamMemoryType::Scale:
            transferSize *= 2;
            break;
        case StreamMemoryType::Triad:
            transferSize *= 3;
            break;
        default:
            break;
        }

        if (arguments.partialMultiplier > 1u) {
            transferSize = transferSize / arguments.partialMultiplier;
        }

        if (eventForEnqueue) {
            cl_ulong timeNs{};
            ASSERT_CL_SUCCESS(ProfilingHelper::getEventDurationInNanoseconds(profilingEvent, timeNs));
            ASSERT_CL_SUCCESS(clReleaseEvent(profilingEvent));

            statistics.pushValue(std::chrono::nanoseconds(timeNs), transferSize, typeSelector.getUnit(), typeSelector.getType());
        } else {
            statistics.pushValue(timer.get(), transferSize, typeSelector.getUnit(), typeSelector.getType());
        }

        if (verify) {
            result = StreamMemoryVerification::verifyOutput(expected, bufferSize, readChunk);
            if (result != TestResult::Success) {
                break;
            }
        }
    }

    // Cleanup
    for (size_t i = 0; i < buffersCount; i++) {
        ASSERT_CL_SUCCESS(clReleaseMemObject(buffers[i]));
    }
    ASSERT_CL_SUCCESS(clReleaseKernel(kernel));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));
    return result;
}

static RegisterTestCaseImplementation<StreamMemory> registerTestCase(run, Api::OpenCL);
