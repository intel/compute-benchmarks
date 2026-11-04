/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/utility/cpu_time_timer.h"

CpuTimeTimer::CpuTimeTimer(Scope scope) : scope(scope) {}

void CpuTimeTimer::measureStart() {
    this->start = getCpuTime(this->scope);
}

void CpuTimeTimer::measureEnd() {
    this->end = getCpuTime(this->scope);
}

std::chrono::nanoseconds CpuTimeTimer::get() const {
    if (this->end <= this->start) {
        return std::chrono::nanoseconds(0);
    }
    return this->end - this->start;
}
