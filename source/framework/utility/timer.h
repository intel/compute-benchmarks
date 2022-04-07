/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include <chrono>
#include <cstdio>
#include <framework/configuration.h>

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

        startTime = Clock::now();
    }

    void measureEnd() {
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
