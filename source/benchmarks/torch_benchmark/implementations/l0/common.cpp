/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "common.hpp"

#include "framework/l0/utility/kernel_helper_l0.h"

Kernel::Kernel(LevelZero &l0,
               const std::string &kernelFileName,
               const std::string &kernelName) {

    TestResult result = L0::KernelHelper::loadKernel(l0, kernelFileName, kernelName, &kernel, &module, nullptr);

    if (result == TestResult::KernelNotFound) {
        throw std::runtime_error("Kernel file not found: " + kernelFileName);
    } else if (result == TestResult::KernelBuildError) {
        throw std::runtime_error("Failed to build kernel: " + kernelName);
    } else if (result != TestResult::Success) {
        throw std::runtime_error("Failed to create kernel: " + kernelName);
    }
}

Kernel::~Kernel() {
    zeKernelDestroy(kernel);
    zeModuleDestroy(module);
}

ze_kernel_handle_t Kernel::get() const {
    return kernel;
}

CommandList::CommandList(ze_context_handle_t context, ze_device_handle_t device, const ze_command_queue_desc_t &desc) {
    if (zeCommandListCreateImmediate(context, device, &desc, &cmdList) != ZE_RESULT_SUCCESS) {
        throw std::runtime_error("Failed to create immediate command list");
    }
}

CommandList::~CommandList() {
    if (cmdList) {
        zeCommandListDestroy(cmdList);
    }
}

Graph::Graph(LevelZero &l0) : l0(l0) {
    if (l0.graphExtension.graphCreate(l0.context, &graph, nullptr) != ZE_RESULT_SUCCESS) {
        throw std::runtime_error("Failed to create graph");
    }
}

Graph::~Graph() {
    if (executableGraph) {
        l0.graphExtension.executableGraphDestroy(executableGraph);
    }
    if (graph) {
        l0.graphExtension.graphDestroy(graph);
    }
}

TestResult Graph::instantiate() {
    if (l0.graphExtension.commandListInstantiateGraph(graph, &executableGraph, nullptr) != ZE_RESULT_SUCCESS) {
        return TestResult::Error;
    }
    return TestResult::Success;
}

CounterBasedEvent::CounterBasedEvent(LevelZero &l0, bool enableProfiling) {
    zex_counter_based_event_desc_t desc = defaultCounterBasedEventDesc;
    desc.flags |= enableProfiling ? ZEX_COUNTER_BASED_EVENT_FLAG_KERNEL_TIMESTAMP : 0;

    if (l0.counterBasedEventCreate2(l0.context, l0.device, &desc, &event) != ZE_RESULT_SUCCESS) {
        throw std::runtime_error("Failed to create counter-based event");
    }
}

CounterBasedEvent::~CounterBasedEvent() {
    if (event) {
        zeEventDestroy(event);
    }
}
