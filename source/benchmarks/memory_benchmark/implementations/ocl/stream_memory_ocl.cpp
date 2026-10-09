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
#include "framework/utility/stream_memory_helper.h"
#include "framework/utility/timer.h"

#include "definitions/stream_memory.h"

#include <algorithm>
#include <gtest/gtest.h>
#include <optional>
#include <vector>

using namespace OCL;

using namespace MemoryConstants;

static TestResult run(const StreamMemoryArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::GigabytesPerSecond, arguments.useEvents ? MeasurementType::Gpu : MeasurementType::Cpu);

    if (isStreamMemoryUnsupported(arguments)) {
        return TestResult::NoImplementation;
    }

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    cl_int retVal = {};
    QueueProperties queueProperties = QueueProperties::create().setProfiling(true).setOoq(0);
    Opencl opencl(queueProperties);
    Timer timer;

    size_t maxWorkgroupSize = {};
    clGetDeviceInfo(opencl.device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(maxWorkgroupSize), &maxWorkgroupSize, nullptr);
    if (arguments.lws > maxWorkgroupSize) {
        return TestResult::DeviceNotCapable;
    }

    const auto scalarValue = getStreamMemoryScalarValue();
    const bool printBuildInfo = true;

    StreamMemoryDispatch dispatch{};
    if (const TestResult result = getStreamMemoryDispatch(arguments, dispatch); result != TestResult::Success) {
        return result;
    }
    const bool verify = Configuration::get().verify;
    const size_t readOutputSize = verify ? dispatch.bufferSize : StreamMemoryHelper::dummyOutputSize;
    const char *kernelName = getStreamMemoryKernelName(arguments, verify);
    const bool setScalarArgument = streamMemoryHasScalarArgument(arguments);
    cl_mem buffers[3] = {};
    size_t buffersCount = {};
    size_t bufferSizes[3] = {dispatch.bufferSize, dispatch.bufferSize, dispatch.bufferSize};

    auto bufferFlags = CL_MEM_READ_WRITE;
    if (arguments.memoryPlacement == UsmRuntimeMemoryPlacement::Host) {
        bufferFlags |= CL_MEM_FORCE_HOST_MEMORY_INTEL;
    }

    switch (arguments.type) {
    case StreamMemoryType::Read:
        buffers[buffersCount++] = clCreateBuffer(opencl.context, bufferFlags, dispatch.bufferSize, nullptr, &retVal);
        ASSERT_CL_SUCCESS(retVal);
        bufferSizes[buffersCount] = readOutputSize;
        buffers[buffersCount++] = clCreateBuffer(opencl.context, bufferFlags, readOutputSize, nullptr, &retVal);
        ASSERT_CL_SUCCESS(retVal);
        break;
    case StreamMemoryType::Triad:
        buffers[buffersCount++] = clCreateBuffer(opencl.context, bufferFlags, dispatch.bufferSize, nullptr, &retVal);
        [[fallthrough]];
    case StreamMemoryType::Scale:
        buffers[buffersCount++] = clCreateBuffer(opencl.context, bufferFlags, dispatch.bufferSize, nullptr, &retVal);
        [[fallthrough]];
    case StreamMemoryType::Write:
        buffers[buffersCount++] = clCreateBuffer(opencl.context, bufferFlags, dispatch.bufferSize, nullptr, &retVal);
        break;
    default:
        FATAL_ERROR("Unknown StreamMemoryType");
    }

    CompilerOptionsBuilder compilerOptions;
    compilerOptions.addDefinitionKeyValue("STREAM_TYPE", getStreamMemoryTypeDefinition(arguments.vectorSize));
    compilerOptions.addDefinitionKeyValue("WRITE_RANDOM_PATTERN", StreamMemoryHelper::getWriteRandomPatternDefinition());
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
    const size_t outputIndex = buffersCount - 1;
    for (auto i = 0u; i < buffersCount; i++) {
        if (verify && arguments.type == StreamMemoryType::Triad && i == 1 && arguments.contents == BufferContents::Random) {
            const auto y = StreamMemoryHelper::getInput(arguments.contents, 1);
            ASSERT_CL_SUCCESS(BufferContentsHelperOcl::fillBufferWithData(opencl.commandQueue, buffers[i], bufferSizes[i], [&](uint8_t *data) { y.copy(data, 0, bufferSizes[i]); }));
        } else {
            ASSERT_CL_SUCCESS(BufferContentsHelperOcl::fillBuffer(opencl.commandQueue, buffers[i], bufferSizes[i], arguments.contents));
        }
        ASSERT_CL_SUCCESS(clSetKernelArg(kernel, static_cast<cl_uint>(i), sizeof(buffers[i]), &buffers[i]))
    }
    if (setScalarArgument) {
        ASSERT_CL_SUCCESS(clSetKernelArg(kernel, static_cast<cl_uint>(buffersCount), dispatch.elementSize, scalarValue.data()));
    }

    int multiplier = static_cast<int>(arguments.partialMultiplier);
    if ((arguments.type == StreamMemoryType::Write && arguments.contents != BufferContents::Random) || arguments.type == StreamMemoryType::Read) {
        ASSERT_CL_SUCCESS(clSetKernelArg(kernel, static_cast<cl_uint>(setScalarArgument ? buffersCount + 1 : buffersCount), 4u, &multiplier));
    }

    ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &dispatch.globalSize, &dispatch.groupSize, 0, nullptr, nullptr));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));

    std::vector<uint8_t> outputChunk;
    std::optional<StreamMemoryHelper::Verifier> verifier;
    if (verify) {
        outputChunk.resize(std::min(dispatch.bufferSize, VerificationHelper::chunkSize));
        verifier.emplace(arguments.type, arguments.contents, dispatch.bufferSize, dispatch.elementSize, arguments.partialMultiplier, scalarValue[0]);
    }
    const auto readChunk = [&](size_t offset, size_t size) -> const uint8_t * {
        CL_SUCCESS_OR_RETURN_VALUE(clEnqueueReadBuffer(opencl.commandQueue, buffers[outputIndex], CL_BLOCKING, offset, size, outputChunk.data(), 0, nullptr, nullptr), nullptr);
        return outputChunk.data();
    };
    TestResult result = TestResult::Success;

    for (auto i = 0u; i < arguments.iterations; i++) {
        if (verify) {
            ASSERT_CL_SUCCESS(clEnqueueFillBuffer(opencl.commandQueue, buffers[outputIndex], &VerificationHelper::poisonByte, sizeof(VerificationHelper::poisonByte), 0, bufferSizes[outputIndex], 0, nullptr, nullptr));
            ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
        }
        cl_event profilingEvent{};
        cl_event *eventForEnqueue = arguments.useEvents ? &profilingEvent : nullptr;

        timer.measureStart();
        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &dispatch.globalSize, &dispatch.groupSize, 0, nullptr, eventForEnqueue));
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
        timer.measureEnd();

        const size_t transferSize = getStreamMemoryTransferSize(arguments, dispatch.bufferSize);

        if (eventForEnqueue) {
            cl_ulong timeNs{};
            ASSERT_CL_SUCCESS(ProfilingHelper::getEventDurationInNanoseconds(profilingEvent, timeNs));
            ASSERT_CL_SUCCESS(clReleaseEvent(profilingEvent));

            statistics.pushValue(std::chrono::nanoseconds(timeNs), transferSize, typeSelector.getUnit(), typeSelector.getType());
        } else {
            statistics.pushValue(timer.get(), transferSize, typeSelector.getUnit(), typeSelector.getType());
        }

        if (verify) {
            result = verifier->verify(readChunk);
            if (result != TestResult::Success) {
                break;
            }
        }
    }

    for (size_t i = 0; i < buffersCount; i++) {
        ASSERT_CL_SUCCESS(clReleaseMemObject(buffers[i]));
    }
    ASSERT_CL_SUCCESS(clReleaseKernel(kernel));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));
    return result;
}

static RegisterTestCaseImplementation<StreamMemory> registerTestCase(run, Api::OpenCL);
