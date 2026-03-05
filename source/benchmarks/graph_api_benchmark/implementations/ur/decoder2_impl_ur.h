/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/ur/ur.h"
#include "framework/utility/file_helper.h"

#include "definitions/decoder2_graph_base.h"

#include <memory>

#if __has_include(<unified-runtime/ur_api.h>)
#include <unified-runtime/ur_api.h>
#else
#include <ur_api.h>
#endif

class Decoder2GraphUR : public Decoder2GraphBase<Decoder2GraphUR> {
  public:
    Decoder2GraphUR(const Decoder2GraphArguments &arguments)
        : Decoder2GraphBase<Decoder2GraphUR>(arguments) {
    }

    DataIntPtr allocDevice(uint32_t count);
    TestResult clearDeviceBuffer(int *devicePtr, uint32_t count);

    TestResult init();
    TestResult destroy();
    TestResult recordGraph();
    TestResult runGraph();
    TestResult waitCompletion();
    TestResult readResults(int *actualSum, int *actualSignalCount);
    bool isUnsupported();

    TestResult runLayer();
    TestResult runAllLayers();

  private:
    std::shared_ptr<UrState> urState;

    ur_program_handle_t program{};
    ur_kernel_handle_t kernel{};
    ur_queue_handle_t queue{};
    ur_exp_command_buffer_handle_t cmdBuffer{};

    static constexpr size_t globalWorkGroupSize[3] = {1, 1, 1};
    static constexpr size_t globalOffset = 0;
    static constexpr size_t nDimensions = 3;
};
