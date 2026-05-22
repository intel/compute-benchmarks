/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "decoder2_impl_sycl.h"

#include "framework/sycl/sycl.h"
#include "framework/test_case/test_case.h"

#include <memory>
#include <sycl/ext/oneapi/backend/level_zero.hpp>
#include <sycl/sycl.hpp>
#include <variant>

Decoder2GraphSYCL::DataIntPtr Decoder2GraphSYCL::allocDevice(uint32_t count) {
    int *devicePtr = sycl::malloc_device<int>(count, *queue);
    clearDeviceBuffer(devicePtr, count);
    auto copied = queue;
    return Decoder2GraphSYCL::DataIntPtr(devicePtr, [copied](int *ptr) {
        sycl::free(ptr, *copied);
    });
}

TestResult Decoder2GraphSYCL::clearDeviceBuffer(int *devicePtr, uint32_t count) {
    queue->memset(devicePtr, 0, count * sizeof(int)).wait();
    return TestResult::Success;
}

TestResult Decoder2GraphSYCL::init() {
    queue = std::make_shared<sycl::queue>(sycl::gpu_selector_v, sycl::property_list{sycl::property::queue::in_order{}});

    if (!queue->get_device().has(sycl::aspect::ext_oneapi_limited_graph)) {
        return TestResult::DeviceNotCapable;
    }

    if (!queue->get_device().has(sycl::aspect::usm_host_allocations) &&
        !queue->get_device().has(sycl::aspect::usm_device_allocations)) {
        return TestResult::DeviceNotCapable;
    }

    if ((useGraphs && useNativeRecording && useHostTasks)) {
        // Get the L0 driver from the SYCL platform and load zeCommandListAppendHostFunction.
        // This replaces SYCL host_task, which is unsupported in native recording mode.
        ze_driver_handle_t driver = sycl::get_native<sycl::backend::ext_oneapi_level_zero>(queue->get_device().get_platform());
        zeDriverGetExtensionFunctionAddress(driver, "zeCommandListAppendHostFunction",
                                            reinterpret_cast<void **>(&zeCommandListAppendHostFunction));

        // Retrieve the native immediate command list for the in-order SYCL queue.
        // Must be obtained before begin_recording; the same handle is used during recording.
        auto nativeQ = sycl::get_native<sycl::backend::ext_oneapi_level_zero>(*queue);
        if (std::holds_alternative<ze_command_list_handle_t>(nativeQ)) {
            zeCommandList = std::get<ze_command_list_handle_t>(nativeQ);
        }
        if (!zeCommandListAppendHostFunction || !zeCommandList) {
            return TestResult::ApiNotCapable;
        }
    }

    return TestResult::Success;
}

TestResult Decoder2GraphSYCL::readResults(int *actualSum, int *actualSignalCount) {
    // Read the results from the device
    queue->memcpy(actualSum, graphData.get(), sizeof(int)).wait();
    *actualSignalCount = *(com.canBegin);
    return TestResult::Success;
}

TestResult Decoder2GraphSYCL::destroy() {
    return TestResult::Success;
}

TestResult Decoder2GraphSYCL::runLayer() {
    int *data = graphData.get();
    uint32_t numIncrements = INCREMENTS_PER_KERNEL;
    for (uint32_t k = 0; k < KERNELS_PER_LAYER; ++k) {
        sycl_ext::submit(*queue, [&](sycl::handler &cgh) {
            sycl_ext::single_task<class decoder2>(cgh, [=]() {
                for (uint32_t j = 0; j < numIncrements; ++j)
                    *data = *data + 1;
            });
        });
    }
    return TestResult::Success;
}

TestResult Decoder2GraphSYCL::runAllLayers() {
    for (uint32_t i = 0; i < LAYER_NUM; ++i) {
        runLayer();
        // Tell the CPU that it can progress in its work
        queue->submit([&](sycl::handler &cgh) {
            cgh.host_task([this]() {
                gpuHostTask();
            });
        });
    }
    return TestResult::Success;
}

TestResult Decoder2GraphSYCL::runAllLayersNative() {
    auto callback = [](void *userData) { static_cast<Decoder2GraphSYCL *>(userData)->gpuHostTask(); };
    for (uint32_t i = 0; i < LAYER_NUM; ++i) {
        runLayer();
        zeCommandListAppendHostFunction(zeCommandList, reinterpret_cast<void *>(+callback), this, nullptr, nullptr, 0, nullptr);
    }
    return TestResult::Success;
}

TestResult Decoder2GraphSYCL::recordGraph() {
    // No-op if eager execution is used
    if (useGraphs) {
        sycl::property_list graphProps = useNativeRecording
                                             ? sycl::property_list{sycl_ext::property::graph::enable_native_recording{}}
                                             : sycl::property_list{};
        graph = sycl_ext::command_graph<sycl_ext::graph_state::modifiable>{queue->get_context(), queue->get_device(), graphProps};

        graph->begin_recording(*queue);
        if (useHostTasks) {
            if (useNativeRecording)
                runAllLayersNative(); // zeCommandListAppendHostFunction instead of host_task
            else
                runAllLayers(); // SYCL host_task
        } else {
            runLayer();
        }
        graph->end_recording();

        execGraph = sycl_ext::command_graph<sycl_ext::graph_state::executable>(graph->finalize());
    }

    return TestResult::Success;
}

TestResult Decoder2GraphSYCL::waitCompletion() {
    queue->wait();

    return TestResult::Success;
}

TestResult Decoder2GraphSYCL::runGraph() {
    if (useGraphs) {
        sycl_ext::execute_graph(*queue, *execGraph);
    } else {
        if (useHostTasks)
            runAllLayers();
        else
            runLayer();
    }

    return TestResult::Success;
}

bool Decoder2GraphSYCL::isUnsupported() {
    // Emulation mode is for L0 only
    return emulateGraphs;
}
