/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <framework/configuration.h>
#if defined(__ARM_ARCH)
#include <sse2neon.h>
#else
#include <emmintrin.h>
#endif

#if defined(_WIN32)
struct PerfLib {
    void start() {
    }
    uint64_t end() {
        return 0;
    }
};

inline PerfLib &Perf() {
    static PerfLib perf;
    return perf;
}
#else
#include <assert.h>
#include <dlfcn.h>
#include <linux/hw_breakpoint.h> /* Definition of HW_* constants */
#include <linux/perf_event.h>    /* Definition of PERF_* constants */
#include <sys/ioctl.h>
#include <sys/syscall.h> /* Definition of SYS_* constants */
#include <unistd.h>

static long
perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                int cpu, int group_fd, unsigned long flags) {
    // in case of getting permission denied one should call as root:
    // sysctl -w kernel.perf_event_paranoid=0
    return syscall(__NR_perf_event_open, hw_event, pid, cpu,
                   group_fd, flags);
}

inline bool excludeKernelEvents = []() {
    auto env = getenv("BENCHMARKS_PERF_EXCLUDE_KERNEL_EVENTS");
    if (env) {
        return std::stoi(env);
    }
    return 1;
}();

struct PerfLib {
    PerfLib() {
        struct perf_event_attr performanceEvent;
        memset(&performanceEvent, 0, sizeof(struct perf_event_attr));
        performanceEvent.type = PERF_TYPE_HARDWARE;
        performanceEvent.size = sizeof(struct perf_event_attr);
        performanceEvent.config = PERF_COUNT_HW_INSTRUCTIONS;
        performanceEvent.disabled = 1;
        performanceEvent.exclude_kernel = excludeKernelEvents;
        // Don't count hypervisor events.
        performanceEvent.exclude_hv = 1;
        fd = perf_event_open(&performanceEvent, 0, -1, -1, 0);
        if (fd == -1) {
            fprintf(stderr, "Error opening leader %llx: %s\n", performanceEvent.config, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
    void start() {
        ioctl(fd, PERF_EVENT_IOC_RESET, 0);
        ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
    }
    uint64_t end() {
        uint64_t count = 0;
        ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
        auto ret = read(fd, &count, sizeof(uint64_t));
        if (ret < 0) {
            fprintf(stderr, "Error reading counter\n");
            exit(EXIT_FAILURE);
        }
        return count;
    }
    int fd;
};

inline PerfLib &Perf() {
    static PerfLib perf;
    return perf;
}
#endif

class CpuCounter {
  public:
    CpuCounter() {
        if (Configuration::get().markTimers) {
            markTimers = true;
        }
    }

    void measureStart() {
        if (this->markTimers) {
            printf("\n CPU counter START \n");
        }
        // make sure that any pending instructions are done and all memory transactions committed.
        _mm_mfence();
        _mm_lfence();

        Perf().start();
    }

    void measureEnd() {
        // make sure that any pending instructions are done and all memory transactions committed.
        _mm_mfence();
        _mm_lfence();

        events = Perf().end();
        if (this->markTimers) {
            printf("\n CPU counter END \n");
        }
    }

    uint64_t get() const {
        return events;
    }

  private:
    bool markTimers = false;
    uint64_t events;
};
