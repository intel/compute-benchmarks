/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"
#include "framework/utility/memory_constants.h"

struct UsmEUReduceCopyRingArguments : TestCaseArgumentContainer {
    IntegerArgument numDevices;
    ByteSizeArgument size;
    BooleanArgument useEvents;
    IntegerArgument throttledWorkItems;
    ByteSizeArgument tmpBufferSize;

    UsmEUReduceCopyRingArguments()
        : numDevices(*this, "numDevices", "Number of root devices in the ring. Test is skipped when the system has fewer devices"),
          size(*this, "size", "Size of the allreduce message on every device. With throttledWorkItems it is the size reduced and written to the peer by each device"),
          useEvents(*this, "useEvents", "Report the longest time across devices from the start of the first kernel to the end of the last kernel, measured with GPU timestamps. Otherwise report the CPU time from releasing the kernels until all devices complete"),
          throttledWorkItems(*this, "throttledWorkItems", "If not 0, run instead of the allreduce one throttled reduce-copy kernel from p2p_benchmark_reduce_copy_throttled.cl with this many work-items per device. The work-items loop over the message and after every iteration one thread per work-group pauses while the work-group waits on a barrier, so this count sets the rate of peer writes"),
          tmpBufferSize(*this, "tmpBufferSize", "Size of the temporary buffer on every device. The message is moved in chunks of tmpBufferSize / max(numDevices, 3). The default is 384MB") {
        tmpBufferSize = 384 * MemoryConstants::megaByte;
    }
};

struct UsmEUReduceCopyRing : TestCase<UsmEUReduceCopyRingArguments> {
    using TestCase<UsmEUReduceCopyRingArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmEUReduceCopyRing";
    }

    std::string getHelp() const override {
        return "connects numDevices root devices in a ring and runs on every device the kernels of a large message allreduce "
               "ring with EU writes to the peer. The message is split in numDevices parts and each part is moved in chunks. "
               "For every chunk a copy kernel writes a part to a temporary buffer on the next device, a barrier kernel waits "
               "for the previous device, and a reduce-copy kernel computes out1 = in1 + in2 in local device memory and writes "
               "out1 to the next device. With more than 2 devices the barrier and the reduce-copy repeat for every ring step "
               "and copy kernels for the allgather follow. The kernels are compiled at run time from "
               "p2p_benchmark_reduce_copy.cl, which can be modified next to the binary without rebuilding. All devices start "
               "together, so both directions of every link carry writes at the same time. Reports the bandwidth of the peer "
               "writes of one device, which is the allreduce bus bandwidth. With throttledWorkItems one reduce-copy kernel "
               "over the whole message runs with rate limited peer writes, which avoids the collapse of two-way EU peer writes "
               "and shows the peak bandwidth of the link. The best count is just below the collapse and depends on the system.";
    }
};
