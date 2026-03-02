/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/error.h"
#include "framework/test_case/register_test_case.h"
#include "framework/test_case/test_result.h"
#include "framework/utility/combo_profiler.h"
#include "framework/utility/file_helper.h"

#include <level_zero/ze_api.h>
#include <level_zero/zer_api.h>

constexpr float epsilon = 1e-5f;

class Kernel {
    ze_kernel_handle_t kernel{};
    ze_module_handle_t module{};

  public:
    Kernel(LevelZero &l0, const std::string &kernelFileName, const std::string &kernelName);
    ~Kernel();

    ze_kernel_handle_t get() const;
};

class CommandList {
    ze_command_list_handle_t cmdList{};

  public:
    CommandList(ze_context_handle_t context, ze_device_handle_t device, const ze_command_queue_desc_t &desc);
    ~CommandList();

    CommandList(const CommandList &) = delete;
    CommandList &operator=(const CommandList &) = delete;

    ze_command_list_handle_t get() const { return cmdList; }
};

template <typename T>
class DeviceMemory {
    ze_context_handle_t context;
    T *ptr;

  public:
    DeviceMemory(LevelZero &l0, size_t length) : context(l0.context), ptr(nullptr) {
        if (zeMemAllocDevice(l0.context, &zeDefaultGPUDeviceMemAllocDesc,
                             length * sizeof(T), alignof(T), l0.device,
                             (void **)&ptr) != ZE_RESULT_SUCCESS) {
            throw std::bad_alloc();
        }
    }

    ~DeviceMemory() {
        if (ptr) {
            zeMemFree(context, ptr);
        }
    }

    DeviceMemory(const DeviceMemory &) = delete;
    DeviceMemory &operator=(const DeviceMemory &) = delete;

    DeviceMemory(DeviceMemory &&other) noexcept
        : context(other.context), ptr(other.ptr) {
        other.ptr = nullptr;
    }

    DeviceMemory &operator=(DeviceMemory &&other) noexcept {
        if (this != &other) {
            if (ptr)
                zeMemFree(context, ptr);
            context = other.context;
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }

    T *getPtr() { return ptr; }
    T **getAddress() { return &ptr; }
};

template <typename T>
class HostMemory {
    ze_context_handle_t context;
    T *ptr;

  public:
    HostMemory(LevelZero &l0, size_t length) : context(l0.context), ptr(nullptr) {
        if (zeMemAllocHost(l0.context, &zeDefaultGPUHostMemAllocDesc,
                           length * sizeof(T), alignof(T),
                           (void **)&ptr) != ZE_RESULT_SUCCESS) {
            throw std::bad_alloc();
        }
    }

    ~HostMemory() {
        if (ptr) {
            zeMemFree(context, ptr);
        }
    }

    HostMemory(const HostMemory &) = delete;
    HostMemory &operator=(const HostMemory &) = delete;

    HostMemory(HostMemory &&other) noexcept
        : context(other.context), ptr(other.ptr) {
        other.ptr = nullptr;
    }

    HostMemory &operator=(HostMemory &&other) noexcept {
        if (this != &other) {
            if (ptr)
                zeMemFree(context, ptr);
            context = other.context;
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }

    T *getPtr() { return ptr; }
    T **getAddress() { return &ptr; }
};

class Graph {
    LevelZero &l0;
    ze_graph_handle_t graph{};
    ze_executable_graph_handle_t executableGraph{};

  public:
    Graph(LevelZero &l0);
    ~Graph();

    ze_graph_handle_t get() const { return graph; }
    ze_graph_handle_t *getAddress() { return &graph; }
    TestResult instantiate();
    ze_executable_graph_handle_t getExecutable() const { return executableGraph; }
};

class CounterBasedEvent {
    ze_event_handle_t event{};

  public:
    CounterBasedEvent(LevelZero &l0, bool enableProfiling = false);
    ~CounterBasedEvent();

    CounterBasedEvent(const CounterBasedEvent &) = delete;
    CounterBasedEvent &operator=(const CounterBasedEvent &) = delete;

    CounterBasedEvent(CounterBasedEvent &&other) noexcept
        : event(other.event) {
        other.event = nullptr;
    }

    CounterBasedEvent &operator=(CounterBasedEvent &&other) noexcept {
        if (this != &other) {
            if (event) {
                zeEventDestroy(event);
            }
            event = other.event;
            other.event = nullptr;
        }
        return *this;
    }

    ze_event_handle_t get() const { return event; }
    ze_event_handle_t *getAddress() { return &event; }
};
