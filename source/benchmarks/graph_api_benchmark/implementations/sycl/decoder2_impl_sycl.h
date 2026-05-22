/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/l0/extension_properties.h"

#include "definitions/decoder2_graph_base.h"

#include <level_zero/ze_api.h>
#include <memory>
#include <optional>
#include <sycl/sycl.hpp>

namespace sycl_ext = sycl::ext::oneapi::experimental;
class Decoder2GraphSYCL : public Decoder2GraphBase<Decoder2GraphSYCL> {
  public:
    Decoder2GraphSYCL(const Decoder2GraphArguments &arguments)
        : Decoder2GraphBase<Decoder2GraphSYCL>(arguments), queue(nullptr) {
    }

    DataIntPtr allocDevice(uint32_t count);
    TestResult clearDeviceBuffer(int *devicePtr, uint32_t count);

    TestResult init();
    TestResult destroy();
    TestResult recordGraph();
    TestResult runGraph();
    TestResult waitCompletion();
    TestResult readResults(int *actualSum, int *actualSignalCount);

    TestResult runLayer();
    TestResult runAllLayers();
    TestResult runAllLayersNative();
    bool isUnsupported();

  private:
    std::shared_ptr<sycl::queue> queue;

    std::optional<sycl_ext::command_graph<sycl_ext::graph_state::modifiable>> graph;
    std::optional<sycl_ext::command_graph<sycl_ext::graph_state::executable>> execGraph;

    L0::L0CommandListAppendHostFunction zeCommandListAppendHostFunction = nullptr;
    ze_command_list_handle_t zeCommandList = nullptr;
};
