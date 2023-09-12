/*
 * Copyright (C) 2023 Intel Corporation
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
    Bcast,
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
