/*
 * Copyright (C) 2022-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/enum/api.h"
#include "framework/enum/device_selection.h"
#include "framework/enum/math_operation.h"

#include <gtest/gtest.h>

namespace CommonGtestArgs {
inline auto allApis() {
    return ::testing::Values(Api::OpenCL, Api::L0, Api::SYCL, Api::OMP, Api::UR);
}

inline auto workgroupCount() {
    return ::testing::Values(1, 1000, 10000);
}

inline auto workgroupSize() {
    return ::testing::Values(1, 32, 256);
}

inline auto contextDeviceSelections() {
    return ::testing::Values(DeviceSelection::Root,                                                    // 1. Naive
                             DeviceSelection::Tile0,                                                   // 2. Ninja
                             DeviceSelection::Tile1,                                                   // 3. Ninja
                             DeviceSelection::Tile0 | DeviceSelection::Tile1,                          // 4. Ninja
                             DeviceSelection::Root | DeviceSelection::Tile0 | DeviceSelection::Tile1); // 5. Convenience
}

inline auto resourceDeviceSelections() {
    return ::testing::Values(DeviceSelection::Root,
                             DeviceSelection::Tile0,
                             DeviceSelection::Tile1);
}

inline auto usmDeviceSelections() {
    return ::testing::Values(DeviceSelection::Host,                           // Host
                             DeviceSelection::Root,                           // Device
                             DeviceSelection::Tile0,                          // Device
                             DeviceSelection::Tile1,                          // Device
                             DeviceSelection::Host | DeviceSelection::Root,   // Shared
                             DeviceSelection::Host | DeviceSelection::Tile0,  // Shared
                             DeviceSelection::Host | DeviceSelection::Tile1); // Shared
}

inline auto usmSharedSelections() {
    return ::testing::Values(DeviceSelection::Host | DeviceSelection::Root,   // Shared
                             DeviceSelection::Host | DeviceSelection::Tile0,  // Shared
                             DeviceSelection::Host | DeviceSelection::Tile1); // Shared
}

inline auto allAtomicMathOperations() {
    return ::testing::Values(MathOperation::Add, MathOperation::Sub,
                             MathOperation::Xchg, MathOperation::CmpXchg,
                             MathOperation::Inc, MathOperation::Dec,
                             MathOperation::Min, MathOperation::Max,
                             MathOperation::And, MathOperation::Or,
                             MathOperation::Xor);
}

inline auto reducedAtomicMathOperations() {
    return ::testing::Values(MathOperation::Add,
                             MathOperation::Xchg,
                             MathOperation::Inc,
                             MathOperation::Min,
                             MathOperation::And);
}

inline auto allNormalMathOperations() {
    return ::testing::Values(MathOperation::Add, MathOperation::Sub,
                             MathOperation::Div, MathOperation::Modulo,
                             MathOperation::Inc, MathOperation::Dec,
                             MathOperation::Min, MathOperation::Max,
                             MathOperation::And, MathOperation::Or,
                             MathOperation::Xor);
}

struct EnqueueSize {
    size_t workgroupCount;
    size_t workgroupSize;
};

inline auto enqueueSizesForAtomics() {
    return ::testing::Values(
        EnqueueSize{1, 1},    // single work item
        EnqueueSize{1, 256},  // one big group
        EnqueueSize{2048, 1}, // 2048 threads in 1024 groups
        EnqueueSize{64, 32},  // 2048 threads in 64 groups
        EnqueueSize{32, 64},  // 2048 threads in 32 groups
        EnqueueSize{8, 256}   // 2048 threads in 8 groups
    );
}

inline auto reducedEnqueueSizesForAtomics() {
    return ::testing::Values(
        EnqueueSize{32, 64});
}

} // namespace CommonGtestArgs
