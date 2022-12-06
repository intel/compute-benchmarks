/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/ocl/utility/profiling_helper.h"
#include "framework/ocl/utility/usm_helper_ocl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "blit_size_assigner.h"
#include "definitions/usm_copy_multiple_blits.h"

#include <gtest/gtest.h>

static TestResult run(const UsmCopyMultipleBlitsArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::GigabytesPerSecond, MeasurementType::Gpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    QueueProperties queueProperties = QueueProperties::create().disable();
    Opencl opencl(queueProperties);
    Timer timer;
    auto clEnqueueMemcpyINTEL = (pfn_clEnqueueMemcpyINTEL)clGetExtensionFunctionAddressForPlatform(opencl.platform, "clEnqueueMemcpyINTEL");
    if (!opencl.getExtensions().isUsmSupported()) {
        return TestResult::DriverFunctionNotFound;
    }

    // Create selected blitter queues
    struct PerQueueData {
        cl_command_queue queue; // freed by the OpenCL class
        std::string name;
        bool isMainCopyEngine;
        cl_event event = nullptr;
        void *copySrc;
        void *copyDst;
        size_t copySize = 0;
    };
    BlitSizeAssigner blitSizeAssigner{arguments.size};
    std::vector<PerQueueData> queues;
    for (size_t blitterIndex : arguments.blitters.getEnabledBits()) {
        const bool isMainCopyEngine = blitterIndex == 0;
        if (isMainCopyEngine) {
            blitSizeAssigner.addMainCopyEngine();
        } else {
            blitSizeAssigner.addLinkCopyEngine();
        }

        const Engine engine = EngineHelper::getBlitterEngineFromIndex(blitterIndex);
        const QueueProperties blitterQueueProperties = QueueProperties::create().setForceEngine(engine).setProfiling(true).allowCreationFail();
        const cl_command_queue queue = opencl.createQueue(blitterQueueProperties);
        if (queue == nullptr) {
            return TestResult::DeviceNotCapable;
        }
        if (!QueueFamiliesHelper::validateCapability(queue, CL_QUEUE_CAPABILITY_TRANSFER_BUFFER_INTEL)) {
            return TestResult::DeviceNotCapable;
        }

        const std::string queueName = EngineHelper::getEngineName(engine);
        queues.push_back(PerQueueData{queue, queueName, isMainCopyEngine});
    }

    // Create buffers
    UsmHelperOcl::Alloc srcAlloc{};
    UsmHelperOcl::Alloc dstAlloc{};
    ASSERT_CL_SUCCESS(UsmHelperOcl::allocate(opencl, arguments.sourcePlacement, arguments.size, srcAlloc));
    ASSERT_CL_SUCCESS(UsmHelperOcl::allocate(opencl, arguments.destinationPlacement, arguments.size, dstAlloc));

    // Calculate copyOffset and copySize for each copy engine
    for (auto i = 0u; i < queues.size(); i++) {
        const auto [offset, size] = blitSizeAssigner.getSpaceForBlit(queues[i].isMainCopyEngine);
        queues[i].copySrc = static_cast<char *>(srcAlloc.ptr) + offset;
        queues[i].copyDst = static_cast<char *>(dstAlloc.ptr) + offset;
        queues[i].copySize = size;
    }
    blitSizeAssigner.validate();

    // Warmup
    for (PerQueueData &queue : queues) {
        ASSERT_CL_SUCCESS(clEnqueueMemcpyINTEL(queue.queue, CL_FALSE, queue.copyDst, queue.copySrc, queue.copySize, 0, nullptr, nullptr));
        ASSERT_CL_SUCCESS(clFlush(queue.queue));
    }
    for (PerQueueData &queue : queues) {
        ASSERT_CL_SUCCESS(clFinish(queue.queue));
    }

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        for (PerQueueData &queue : queues) {
            ASSERT_CL_SUCCESS(clEnqueueMemcpyINTEL(queue.queue, CL_FALSE, queue.copyDst, queue.copySrc, queue.copySize, 0, nullptr, &queue.event));
        }
        for (PerQueueData &queue : queues) {
            ASSERT_CL_SUCCESS(clFlush(queue.queue));
        }
        for (PerQueueData &queue : queues) {
            ASSERT_CL_SUCCESS(clFinish(queue.queue));
        }
        timer.measureEnd();

        // Report individual engines results and get time delta
        std::chrono::nanoseconds endGpuTime{};
        std::chrono::nanoseconds startGpuTime = std::chrono::nanoseconds::duration::max();

        for (PerQueueData &queue : queues) {
            cl_ulong timeNs = 0ul;
            cl_ulong start, end;
            ASSERT_CL_SUCCESS(clGetEventProfilingInfo(queue.event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, nullptr));
            ASSERT_CL_SUCCESS(clGetEventProfilingInfo(queue.event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, nullptr));
            timeNs = end - start;
            ASSERT_CL_SUCCESS(clReleaseEvent(queue.event));

            statistics.pushValue(std::chrono::nanoseconds(timeNs), queue.copySize, typeSelector.getUnit(), typeSelector.getType(), queue.name);

            startGpuTime = std::min(std::chrono::nanoseconds(start), startGpuTime);
            endGpuTime = std::max(std::chrono::nanoseconds(end), endGpuTime);
        }

        // Report total results
        statistics.pushValue(endGpuTime - startGpuTime, arguments.size, typeSelector.getUnit(), typeSelector.getType(), "Total (Gpu)");
        statistics.pushValue(timer.get(), arguments.size, typeSelector.getUnit(), MeasurementType::Cpu, "Total (Cpu)");
    }

    ASSERT_CL_SUCCESS(UsmHelperOcl::deallocate(srcAlloc));
    ASSERT_CL_SUCCESS(UsmHelperOcl::deallocate(dstAlloc));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<UsmCopyMultipleBlits> registerTestCase(run, Api::OpenCL, true);
