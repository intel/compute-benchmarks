/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/ocl/utility/buffer_contents_helper_ocl.h"
#include "framework/ocl/utility/profiling_helper.h"
#include "framework/ocl/utility/program_helper_ocl.h"
#include "framework/ocl/utility/usm_helper_ocl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/compiler_options_builder.h"
#include "framework/utility/timer.h"

#include "definitions/usm_copy_bidirectional_kernel.h"

using namespace OCL;

static TestResult run(const UsmCopyBidirectionalKernelArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::GigabytesPerSecond, arguments.useEvents ? MeasurementType::Gpu : MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    QueueProperties queueProperties = QueueProperties::create().setProfiling(arguments.useEvents).allowCreationFail();
    Opencl opencl(queueProperties);
    if (opencl.commandQueue == nullptr) {
        return TestResult::DeviceNotCapable;
    }
    if (!opencl.getExtensions().isUsmSupported()) {
        return TestResult::DriverFunctionNotFound;
    }
    Timer timer;
    cl_int retVal{};

    UsmHelperOcl::Alloc hostAlloc{};
    UsmHelperOcl::Alloc deviceAlloc{};
    ASSERT_CL_SUCCESS(UsmHelperOcl::allocate(opencl, UsmMemoryPlacement::Host, arguments.size, hostAlloc));
    ASSERT_CL_SUCCESS(UsmHelperOcl::allocate(opencl, UsmMemoryPlacement::Device, arguments.size, deviceAlloc));
    ASSERT_CL_SUCCESS(BufferContentsHelperOcl::fillUsmBufferOrHostPtr(opencl.commandQueue, hostAlloc.ptr, arguments.size, hostAlloc.placement, BufferContents::Random));
    ASSERT_CL_SUCCESS(BufferContentsHelperOcl::fillUsmBufferOrHostPtr(opencl.commandQueue, deviceAlloc.ptr, arguments.size, deviceAlloc.placement, BufferContents::Random));

    CompilerOptionsBuilder compilerOptions;
    compilerOptions.addDefinitionKeyValue("STRIPE_ELEMENTS", UsmCopyBidirectionalKernelArguments::stripeElements);
    cl_program program{};
    if (auto result = ProgramHelperOcl::buildProgramFromSourceFile(opencl.context, opencl.device, "memory_benchmark_usm_copy_bidirectional_kernel.cl", compilerOptions.str().c_str(), program);
        result != TestResult::Success) {
        return result;
    }
    cl_kernel kernel = clCreateKernel(program, "copy_bidirectional_stripes", &retVal);
    ASSERT_CL_SUCCESS(retVal);
    ASSERT_CL_SUCCESS(clSetKernelArgSVMPointer(kernel, 0, hostAlloc.ptr));
    ASSERT_CL_SUCCESS(clSetKernelArgSVMPointer(kernel, 1, deviceAlloc.ptr));

    const size_t globalWorkSize = arguments.size / UsmCopyBidirectionalKernelArguments::elementSize;

    for (auto i = 0u; i < arguments.iterations; i++) {
        cl_event profilingEvent{};
        cl_event *eventForEnqueue = arguments.useEvents ? &profilingEvent : nullptr;

        timer.measureStart();
        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &globalWorkSize, nullptr, 0, nullptr, eventForEnqueue));
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
        timer.measureEnd();

        if (arguments.useEvents) {
            cl_ulong timeNs{};
            ASSERT_CL_SUCCESS(ProfilingHelper::getEventDurationInNanoseconds(profilingEvent, timeNs));
            ASSERT_CL_SUCCESS(clReleaseEvent(profilingEvent));
            statistics.pushValue(std::chrono::nanoseconds(timeNs), arguments.size, typeSelector.getUnit(), typeSelector.getType());
        } else {
            statistics.pushValue(timer.get(), arguments.size, typeSelector.getUnit(), typeSelector.getType());
        }
    }

    ASSERT_CL_SUCCESS(clReleaseKernel(kernel));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));
    ASSERT_CL_SUCCESS(UsmHelperOcl::deallocate(hostAlloc));
    ASSERT_CL_SUCCESS(UsmHelperOcl::deallocate(deviceAlloc));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<UsmCopyBidirectionalKernel> registerTestCase(run, Api::OpenCL);
