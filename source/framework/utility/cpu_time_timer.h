/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <chrono>

class CpuTimeTimer {
  public:
    enum class Scope {
        Thread,
        Process
    };

    explicit CpuTimeTimer(Scope scope);

    void measureStart();
    void measureEnd();
    std::chrono::nanoseconds get() const;

  private:
    static std::chrono::nanoseconds getCpuTime(Scope scope);

    std::chrono::nanoseconds start{};
    std::chrono::nanoseconds end{};
    Scope scope{};
};
