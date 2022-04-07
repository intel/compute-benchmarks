/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/ocl/utility/profiling_helper.h"
#include "framework/utility/timer.h"
#include "framework/workload/register_workload.h"

struct HelloWorldArguments : WorkloadArgumentContainer {
    PositiveIntegerArgument numberOfElements;
    BooleanArgument useEvents;

    HelloWorldArguments()
        : numberOfElements(*this, "numberOfElements", "Number of elements to process"),
          useEvents(*this, "useEvents", "Use events for measuring") {
    }
};

struct HelloWorld : Workload<HelloWorldArguments> {};

TestResult run(const HelloWorldArguments &arguments, Statistics &statistics, WorkloadSynchronization &synchronization, WorkloadIo &io) {
    QueueProperties queueProperties = QueueProperties::create().setProfiling(arguments.useEvents);
    Opencl opencl(queueProperties);
    cl_int retVal{};
    if (opencl.commandQueue == nullptr) {
        return TestResult::DeviceNotCapable;
    }
    Timer timer{};

    // Create kernel
    std::string kernelSource = R"(
       __kernel void addOne(__global uint *results) {
            results[get_global_id(0)]++;
       }
    )";
    const char *pKernelSource = kernelSource.c_str();
    const size_t kernelSizes = kernelSource.size();
    cl_program program = clCreateProgramWithSource(opencl.context, 1, &pKernelSource, &kernelSizes, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    ASSERT_CL_SUCCESS(clBuildProgram(program, 1, &opencl.device, nullptr, nullptr, nullptr));
    cl_kernel kernel = clCreateKernel(program, "addOne", &retVal);
    ASSERT_CL_SUCCESS(retVal);

    // Prepare the buffer with data.
    auto cpuBuffer = std::make_unique<cl_int[]>(arguments.numberOfElements);
    for (auto i = 0u; i < arguments.numberOfElements; i++) {
        cpuBuffer[i] = i;
    }
    const size_t sizeInBytes = arguments.numberOfElements * sizeof(cl_int);
    cl_mem buffer = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeInBytes, cpuBuffer.get(), &retVal);
    ASSERT_CL_SUCCESS(retVal);

    // Warmup kernel
    const size_t gws[1] = {arguments.numberOfElements};
    const size_t *lws = nullptr;
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 0, sizeof(buffer), &buffer));
    ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, gws, lws, 0, nullptr, nullptr));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));

    // Workload
    for (auto i = 0u; i < arguments.iterations; i++) {
        synchronization.synchronize(io);

        cl_event event{};
        cl_event *eventForEnqueue = arguments.useEvents ? &event : nullptr;

        timer.measureStart();
        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, gws, lws, 0, nullptr, eventForEnqueue));
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
        timer.measureEnd();

        if (arguments.useEvents) {
            cl_ulong timeNs{};
            ASSERT_CL_SUCCESS(ProfilingHelper::getEventDurationInNanoseconds(event, timeNs));
            ASSERT_CL_SUCCESS(clReleaseEvent(event));
            statistics.pushValue(std::chrono::nanoseconds{timeNs}, MeasurementUnit::Microseconds, MeasurementType::Gpu);
        } else {
            statistics.pushValue(timer.get(), MeasurementUnit::Microseconds, MeasurementType::Cpu);
        }
    }

    // Validation
    ASSERT_CL_SUCCESS(clEnqueueReadBuffer(opencl.commandQueue, buffer, CL_BLOCKING, 0u, sizeInBytes, cpuBuffer.get(), 0u, nullptr, nullptr));
    for (auto i = 0u; i < arguments.numberOfElements; i++) {
        const cl_int expectedValue = i + 1 + static_cast<cl_int>(arguments.iterations);
        if (cpuBuffer[i] != expectedValue) {
            return TestResult::VerificationFail;
        }
    }

    // Cleanup
    ASSERT_CL_SUCCESS(clReleaseMemObject(buffer));
    ASSERT_CL_SUCCESS(clReleaseKernel(kernel));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));
    return TestResult::Success;
}

int main(int argc, char **argv) {
    HelloWorld workload;
    HelloWorld::implementation = run;
    return workload.runFromCommandLine(argc, argv);
}
