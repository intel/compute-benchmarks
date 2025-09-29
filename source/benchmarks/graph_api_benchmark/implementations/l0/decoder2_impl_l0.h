/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/utility/file_helper.h"

#include "definitions/decoder2_graph_base.h"

#include <memory>

class Decoder2GraphL0 : public Decoder2GraphBase<Decoder2GraphL0> {
  public:
    Decoder2GraphL0(const Decoder2GraphArguments &arguments)
        : Decoder2GraphBase<Decoder2GraphL0>(arguments), levelzero(nullptr) {
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
    std::shared_ptr<LevelZero> levelzero;

    ze_group_count_t groupCount{1u, 1u, 1u};
    ze_kernel_handle_t kernelDecoder2{};
    ze_module_handle_t moduleDecoder2{};
    ze_command_list_handle_t cmdList{};
    // In emulate mode these graph types are unused
    ze_graph_handle_t graph{};
    ze_executable_graph_handle_t execGraph{};
    ze_command_list_handle_t immCmdList{};
};
