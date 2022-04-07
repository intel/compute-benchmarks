/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/utility/error.h"

#include <array>

enum class AtomicMemoryOrder {
    Unknown,
    Relaxed,
    Acquire,
    Release,
    AcquireRelease,
    SequentialConsitent,
};

namespace AtomicMemoryOrderHelper {
constexpr inline std::array<AtomicMemoryOrder, 5> allValues = {
    AtomicMemoryOrder::Relaxed,
    AtomicMemoryOrder::Acquire,
    AtomicMemoryOrder::Release,
    AtomicMemoryOrder::AcquireRelease,
    AtomicMemoryOrder::SequentialConsitent,
};

inline std::string toOpenclC(AtomicMemoryOrder order) {
    switch (order) {
    case AtomicMemoryOrder::Relaxed:
        return "memory_order_relaxed";
    case AtomicMemoryOrder::Acquire:
        return "memory_order_acquire";
    case AtomicMemoryOrder::Release:
        return "memory_order_release";
    case AtomicMemoryOrder::AcquireRelease:
        return "memory_order_acq_rel";
    case AtomicMemoryOrder::SequentialConsitent:
        return "memory_order_seq_cst";
    default:
        FATAL_ERROR("Unknown memory order");
    }
}
} // namespace AtomicMemoryOrderHelper
