/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/sin_kernel_graph_base.h"

#include <iostream>
#include <math.h>
#include <sycl/sycl.hpp>

namespace sycl_ext = sycl::ext::oneapi::experimental;

class SinKernelGraphSYCL : public SinKernelGraphBase {
  public:
    SinKernelGraphSYCL(const SinKernelGraphArguments &arguments)
        : SinKernelGraphBase(arguments), queue(nullptr) {
    }

    ~SinKernelGraphSYCL();

    DataFloatPtr allocDevice(uint32_t count) override;
    DataFloatPtr allocHost(uint32_t count) override;

    TestResult init() override;
    TestResult recordGraph() override;
    TestResult readResults(float *output_h) override;
    TestResult runGraph(float *input_h) override;
    TestResult runEager(float *input_h) override;
    TestResult waitCompletion() override;

    TestResult runKernels();

  private:
    std::shared_ptr<sycl::queue> queue;

    std::optional<sycl_ext::command_graph<sycl_ext::graph_state::modifiable>> graph;
    std::optional<sycl_ext::command_graph<sycl_ext::graph_state::executable>> execGraph;
};
