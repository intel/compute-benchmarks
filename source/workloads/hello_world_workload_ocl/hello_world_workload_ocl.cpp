/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

/**************************************************************************************************
This is an example workload written to help benchmark creators add their own test cases and
understand what needs to be done. Its is meant to serve merely as a code sample and should not be
used to any serious performance analysis.

This file is a step by step guide on what needs to be done to create a workload. The
hello_world_template_workload_ocl.cpp file contains the same code, but without the verbose
comments. It may be used as a template for creating new tests.

If you run this application without parameters, it will tell you which ones you're missing. Example
valid invocation is:
    ./hello_world_workload_ocl --numberOfElements=10000 --useEvents=1
***************************************************************************************************/
#include "framework/ocl/opencl.h"
#include "framework/ocl/utility/profiling_helper.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"
#include "framework/workload/register_workload.h"

/**************************************************************************************************
First thing, that needs to be done is the definition of parameters taken by our workload. Class
which defines this must derive from WorkloadArgumentContainer base class. It is recommended that
this class be named as "<WorkloadName>Arguments" for consistency.

Each parameter is represented by a member variable deriving from the Argument class. A few examples
of common datatypes are provided in the example below. For others, feel free to explore
source/framework/argument directory. Each Argument constructor will take its name and an example
description. The former will be used in command line parsing and the latter will be printed upon
running with --help parameter.

Every argument has to be specified by user in the command-line. The format is '--key=value'. It is
possible to provide default values, though we do not recommend it, because users may not know
it and get different results when default values are changed.
***************************************************************************************************/
struct HelloWorldArguments : WorkloadArgumentContainer {
    PositiveIntegerArgument numberOfElements;
    BooleanArgument useEvents;

    HelloWorldArguments()
        : numberOfElements(*this, "numberOfElements", "Number of elements to process"),
          useEvents(*this, "useEvents", "Use events for measuring") {

        // It is possible to provide default value. Uncomment line above to make 7 a default numberOfElements
        // numberOfElements = 7;
    }
};

/**************************************************************************************************
Second thing, we have to do is to define our workload. This definition has to inherit from the
Workload class with its template argument set to our arguments class, which we've defined above.
***************************************************************************************************/
struct HelloWorld : Workload<HelloWorldArguments> {};

/**************************************************************************************************
Third and probably the most important thing is the implementation of our workload. It is a function
taking a couple of arguments described below:
1. arguments - an instance to our arguments class we've defined
2. statistics - a container for performance results produced by our implementation
3. synchronization - an object encapsulating synchronization. For workloads meant to be run from
        the command line this is unused. This object is used by multiprocess benchmarks, which run
        several workloads as child processes and insert synchronization points inside of them.
        Synchronization should be called at the beginning of each test iteration.
4. io - an object encapsulating printing and reading during execution. Just like synchronization,
        for workloads meant to be run from the command line this is unused. It is required for
        workloads that are run by other processes, because different communication channels are
        used for different purposes (e.g. results are written to a different pipe than debug
        messages). For normal, human-run workloads, everything goes to the console.

Every API call should be checked for error with ASSERT_CL_SUCCESS (or ASSERT_ZE_RESULT_SUCCESS in
LevelZero). If an error is found, the test will be stopped and reported as failed.
***************************************************************************************************/
TestResult run(const HelloWorldArguments &arguments, Statistics &statistics, WorkloadSynchronization &synchronization, WorkloadIo &io) {
    // The OpenCL class is a fundamental class encapsulating cl_platform and cl_device_id selection
    // and creation of cl_context and cl_command_queue. QueueProperties and ContextProperties are
    // optional parameters used to configure these aspects. Example below enables the
    // CL_QUEUE_PROFILING_ENABLE based on boolean value of the --useEvents argument provided by user.
    // For LevelZero there are analogous helper class.
    QueueProperties queueProperties = QueueProperties::create().setProfiling(arguments.useEvents);
    Opencl opencl(queueProperties);
    cl_int retVal{};

    // If we set some flags, which may not be supported, creation may fail. We should check it and
    // returned an apprioprate test error if it does. Refer to the definition of TestResult enum
    // for details.
    if (opencl.commandQueue == nullptr) {
        return TestResult::DeviceNotCapable;
    }

    // Timer class is used for CPU-side timings. If they are not needed (because events are used),
    // we do not need to create this.
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

    // Warmup kernel. Usually we want to perform at least one iteration of warmup run.
    const size_t gws[1] = {arguments.numberOfElements};
    const size_t *lws = nullptr;
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 0, sizeof(buffer), &buffer));
    ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, gws, lws, 0, nullptr, nullptr));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));

    // The actual benchmark. We have to run the exact number of iterations that are provided in
    // arguments.iterations. Each iteration must call statistics.pushValue to provide the
    // result measured in nanoseconds. The framework will validate that and print a message if
    // we do not comply.
    for (auto i = 0u; i < arguments.iterations; i++) {
        synchronization.synchronize(io);

        cl_event event{};
        cl_event *eventForEnqueue = arguments.useEvents ? &event : nullptr;

        timer.measureStart();
        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, gws, lws, 0, nullptr, eventForEnqueue));
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
        timer.measureEnd();

        // We have defined two ways to get results, so we make this check here. If you want to have
        // a single mechanism (events-only or CPU-measurements only), then it will be simpler.
        if (arguments.useEvents) {
            cl_ulong timeNs{};
            ASSERT_CL_SUCCESS(ProfilingHelper::getEventDurationInNanoseconds(event, timeNs));
            ASSERT_CL_SUCCESS(clReleaseEvent(event));
            statistics.pushValue(std::chrono::nanoseconds{timeNs}, MeasurementUnit::Microseconds, MeasurementType::Gpu);
        } else {
            statistics.pushValue(timer.get(), MeasurementUnit::Microseconds, MeasurementType::Cpu);
        }
    }

    // Validate results. This is an optional step.
    ASSERT_CL_SUCCESS(clEnqueueReadBuffer(opencl.commandQueue, buffer, CL_BLOCKING, 0u, sizeInBytes, cpuBuffer.get(), 0u, nullptr, nullptr));
    for (auto i = 0u; i < arguments.numberOfElements; i++) {
        const cl_int expectedValue = i + 1 + static_cast<cl_int>(arguments.iterations);
        if (cpuBuffer[i] != expectedValue) {
            return TestResult::VerificationFail;
        }
    }

    // Perform cleanup
    ASSERT_CL_SUCCESS(clReleaseMemObject(buffer));
    ASSERT_CL_SUCCESS(clReleaseKernel(kernel));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));

    // Report a successful execution
    return TestResult::Success;
}

/**************************************************************************************************
Fourth and the last step is the main() function. Here we make use of everything we defined above.
Command line will be parsed inside the runFromCommandLine and provided implementation function will
be run with parsed parameters.
***************************************************************************************************/
int main(int argc, char **argv) {
    HelloWorld workload;
    HelloWorld::implementation = run;
    return workload.runFromCommandLine(argc, argv);
}
