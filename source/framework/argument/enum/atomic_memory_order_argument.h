/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/enum_argument.h"
#include "framework/enum/atomic_memory_order.h"

struct AtomicMemoryOrderArgument : EnumArgument<AtomicMemoryOrderArgument, AtomicMemoryOrder> {
    using EnumArgument::EnumArgument;
    ThisType &operator=(EnumType newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    const static inline std::string enumName = "atomic memory order";
    const static inline EnumType invalidEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[5] = {AtomicMemoryOrder::Relaxed,
                                                  AtomicMemoryOrder::Acquire,
                                                  AtomicMemoryOrder::Release,
                                                  AtomicMemoryOrder::AcquireRelease,
                                                  AtomicMemoryOrder::SequentialConsitent};
    const static inline std::string enumValuesNames[5] = {"relaxed",
                                                          "acquire",
                                                          "release",
                                                          "acq_rel",
                                                          "seq_cst"};
};
