/*
 * Copyright (C) 2022-2026 Intel Corporation
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

        if (state == State::STARTED) {
            FATAL_ERROR("Timer measureStart called twice without measureEnd in the middle\n");
        }
        state = State::STARTED;

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
        if (state == State::STARTED) {
            state = State::READY;
        } else {
            FATAL_ERROR("Timer measureEnd called without measureStart\n");
        }
        if (this->markTimers) {
            printf("\n Timer END \n");
        }
    }

    Clock::duration get() const {
        if (endTime <= startTime) {
            return std::chrono::nanoseconds(1);
        }
        return endTime - startTime;
    }

    bool measurementIsReady() const { return state == State::READY; }

  private:
    bool markTimers = false;
    Clock::time_point startTime;
    Clock::time_point endTime;

    enum class State { IDLE,
                       STARTED,
                       READY };
    State state = State::IDLE;
};
