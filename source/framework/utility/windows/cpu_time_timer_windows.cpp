/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/utility/cpu_time_timer.h"
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
