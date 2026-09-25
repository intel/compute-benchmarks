/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/stream_memory_type_argument.h"
#include "framework/test_case/test_case.h"

struct HostMappedMemoryArguments : TestCaseArgumentContainer {
    StreamMemoryTypeArgument type;
    ByteSizeArgument size;
    BooleanArgument fenceWrites;

    HostMappedMemoryArguments()
        : type(*this, "type", "Access the CPU performs through the mapping, read or write"),
          size(*this, "size", "Size of the mapped device allocation"),
          fenceWrites(*this, "fenceWrites", "Order writes with a full fence before the measurement ends, as a producer signalling a remote reader has to") {}
};

struct HostMappedMemory : TestCase<HostMappedMemoryArguments> {
    using TestCase<HostMappedMemoryArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "HostMappedMemory";
    }

    std::string getHelp() const override {
        return "maps a device allocation into the host address space with "
               "zeIntelMemMapDeviceMemToHost and measures the bandwidth of a CPU memcpy through "
               "that mapping. On a device with local memory the mapping is reached over the PCIe "
               "BAR and is write-combining, so writes stream while reads become uncached "
               "per-cacheline round trips, which makes the two directions differ by orders of "
               "magnitude. On an integrated device the driver returns the existing host pointer "
               "and the result is an ordinary system memory measurement. Requires a driver "
               "exposing the mapping extension and an allocation the host can alias directly.";
    }
};
