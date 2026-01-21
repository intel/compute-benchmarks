/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <algorithm>
#include <framework/enum/measurement_unit.h>
#include <framework/enum/profiler_type.h>
#include <framework/utility/cpu_counter.h>
#include <framework/utility/statistics.h>
#include <framework/utility/timer.h>
#include <unordered_set>

class ComboProfiler {
  public:
    ComboProfiler(ProfilerType profiler = ProfilerType::Timer)
        : profiler_type(profiler) {
        switch (profiler_type) {
        case ProfilerType::Timer:
            measurement.setUnit(MeasurementUnit::Microseconds);
            measurement.setType(MeasurementType::Cpu);
            break;
        case ProfilerType::CpuCounter:
            measurement.setUnit(MeasurementUnit::CpuHardwareCounter);
            measurement.setType(MeasurementType::Cpu);
            break;
        default:
            FATAL_ERROR("Undefined ProfilerType provided");
        }
    }

    inline void measureStart() {
        switch (profiler_type) {
        case ProfilerType::CpuCounter:
            cpuCounter.measureStart();
            break;
        case ProfilerType::Timer:
            timer.measureStart();
            break;
        default:
            FATAL_ERROR("Undefined ProfilerType provided");
        }
    }

    inline void measureEnd() {
        switch (profiler_type) {
        case ProfilerType::Timer:
            timer.measureEnd();
            break;
        case ProfilerType::CpuCounter:
            cpuCounter.measureEnd();
            break;
        default:
            FATAL_ERROR("Undefined ProfilerType provided");
        }
    }

  protected:
    ProfilerType profiler_type;
    Timer timer;
    CpuCounter cpuCounter;
    MeasurementFields measurement;
};

class ComboProfilerWithStats : public ComboProfiler {
  public:
    ComboProfilerWithStats(ProfilerType profiler = ProfilerType::Timer)
        : ComboProfiler(profiler) {}

    void pushStats(Statistics &statistics) {
        switch (profiler_type) {
        case ProfilerType::Timer:
            if (timer.measurementIsReady()) {
                statistics.pushValue(
                    timer.get(),
                    measurement.getUnit(),
                    measurement.getType(),
                    "time");
            }
            break;
        case ProfilerType::CpuCounter:
            if (cpuCounter.measurementIsReady()) {
                statistics.pushCpuCounter(
                    cpuCounter.get(),
                    measurement.getUnit(),
                    measurement.getType(),
                    "hw instructions");
            }
            break;
        default:
            FATAL_ERROR("Undefined ProfilerType provided");
        }
    }

    void pushNoop(Statistics &statistics) {
        // Push dummy statistic for Noop
        statistics.pushUnitAndType(
            measurement.getUnit(), measurement.getType());
    }
};
