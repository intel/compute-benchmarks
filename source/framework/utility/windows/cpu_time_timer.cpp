/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/utility/windows/cpu_time_timer.h"

#include "framework/utility/error.h"
#include "framework/utility/windows/windows.h"

namespace {
std::chrono::nanoseconds fileTimeToDuration(const FILETIME &fileTime) {
    ULARGE_INTEGER value{};
    value.LowPart = fileTime.dwLowDateTime;
    value.HighPart = fileTime.dwHighDateTime;
    return std::chrono::nanoseconds(static_cast<std::chrono::nanoseconds::rep>(value.QuadPart * 100ull));
}
} // namespace

CpuTimeTimer::CpuTimeTimer(Scope scope) : scope(scope) {}

void CpuTimeTimer::measureStart() {
    start = getCpuTime(scope);
}

void CpuTimeTimer::measureEnd() {
    end = getCpuTime(scope);
}

std::chrono::nanoseconds CpuTimeTimer::get() const {
    if (end <= start) {
        return std::chrono::nanoseconds(0);
    }
    return end - start;
}

std::chrono::nanoseconds CpuTimeTimer::getCpuTime(Scope scope) {
    FILETIME creationTime{};
    FILETIME exitTime{};
    FILETIME kernelTime{};
    FILETIME userTime{};
    const auto querySucceeded = scope == Scope::Thread
                                    ? GetThreadTimes(GetCurrentThread(), &creationTime, &exitTime, &kernelTime, &userTime)
                                    : GetProcessTimes(GetCurrentProcess(), &creationTime, &exitTime, &kernelTime, &userTime);
    FATAL_ERROR_IF(querySucceeded == 0, "CPU time query failed");
    return fileTimeToDuration(kernelTime) + fileTimeToDuration(userTime);
}
