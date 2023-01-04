/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include <chrono>
#include <cstdio>
#include <framework/configuration.h>
#if defined(__ARM_ARCH)
#include <sse2neon.h>
#else
#include <emmintrin.h>
#endif

class Timer {
  public:
    Timer() {
        if (Configuration::get().markTimers) {
            markTimers = true;
        }
    }
    using Clock = std::chrono::high_resolution_clock;

    void measureStart() {
        if (this->markTimers) {
            printf("\n Timer START \n");
        }
        // make sure that any pending instructions are done and all memory transactions committed.
        _mm_mfence();
        _mm_lfence();
        startTime = Clock::now();
    }

    void measureEnd() {
        // make sure that any pending instructions are done and all memory transactions committed.
        _mm_mfence();
        _mm_lfence();
        endTime = Clock::now();
        if (this->markTimers) {
            printf("\n Timer END \n");
        }
    }

    Clock::duration get() const {
        return endTime - startTime;
    }

  private:
    bool markTimers = false;
    Clock::time_point startTime;
    Clock::time_point endTime;
};
