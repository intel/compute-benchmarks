/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/utility/cpu_time_timer.h"
#include "framework/utility/linux/error.h"

#include <time.h>

namespace {
std::chrono::nanoseconds timespecToDuration(const timespec &time) {
    return std::chrono::seconds(time.tv_sec) + std::chrono::nanoseconds(time.tv_nsec);
}
} // namespace

std::chrono::nanoseconds CpuTimeTimer::getCpuTime(Scope scope) {
    const clockid_t clockId = scope == Scope::Thread ? CLOCK_THREAD_CPUTIME_ID : CLOCK_PROCESS_CPUTIME_ID;

    timespec cpuTime{};
    FATAL_ERROR_IF_SYS_CALL_FAILED(clock_gettime(clockId, &cpuTime), "CPU time query failed");
    return timespecToDuration(cpuTime);
}
