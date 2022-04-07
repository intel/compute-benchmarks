/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "framework/utility/error.h"

#include <array>

enum class AtomicScope {
    Unknown,
    Workgroup,
    Device,
};

namespace AtomicScopeHelper {
constexpr inline std::array<AtomicScope, 2> allValues = {
    AtomicScope::Workgroup,
    AtomicScope::Device,
};

inline std::string toOpenclC(AtomicScope order) {
    switch (order) {
    case AtomicScope::Workgroup:
        return "memory_scope_work_group";
    case AtomicScope::Device:
        return "memory_scope_device";
    default:
        FATAL_ERROR("Unknown memory scope");
    }
}
} // namespace AtomicScopeHelper
