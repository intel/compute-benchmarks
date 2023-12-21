/*
 * Copyright (C) 2023-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

enum class MpiTestType : int {
    Unknown,
    Startup,
    Bandwidth,
    Latency,
    Overlap,
    Broadcast,
    Reduce,
    AllReduce,
    AllToAll
};

enum class MpiStatisticsType : int {
    Unknown,
    Avg,
    Max,
    Min
};
