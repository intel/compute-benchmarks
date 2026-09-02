/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/utility/cpu_topology.h"

#include <cerrno>
#include <cpuid.h>
#include <cstring>
#include <sched.h>

namespace CpuTopologyDetail {
CpuidRegisters cpuidEx(uint32_t leaf, uint32_t subLeaf) {
    CpuidRegisters registers{};
    __cpuid_count(leaf, subLeaf, registers.eax, registers.ebx, registers.ecx, registers.edx);
    return registers;
}
} // namespace CpuTopologyDetail

CoreTypeLookupResult getCpuMaskForCoreType(CpuCoreType coreType, uint64_t &outMask, std::string &errorMessage) {
    outMask = 0u;
    if (!CpuTopologyDetail::isIntelHybridCpu(errorMessage)) {
        return CoreTypeLookupResult::NotHybrid;
    }

    // A process can run on more than 64 CPUs; restoring a uint64_t projection would narrow its affinity.
    cpu_set_t originalAffinity;
    CPU_ZERO(&originalAffinity);
    if (sched_getaffinity(0, sizeof(originalAffinity), &originalAffinity) != 0) {
        errorMessage = std::string("sched_getaffinity failed with errno=") + std::strerror(errno);
        return CoreTypeLookupResult::NoMatchingCpu;
    }

    uint64_t processMask = 0u;
    for (uint32_t cpu = 0u; cpu < CpuTopologyDetail::maxDetectableCpuCount; ++cpu) {
        if (CPU_ISSET(cpu, &originalAffinity)) {
            processMask |= 1ull << cpu;
        }
    }

    bool anyCoreTypeReported = false;
    const auto probeSingleCpu = [](uint32_t cpu, CpuCoreType &outCoreType) {
        // sched_setaffinity migrates the calling thread before it returns, so CPUID runs on the requested CPU.
        cpu_set_t probedAffinity;
        CPU_ZERO(&probedAffinity);
        CPU_SET(cpu, &probedAffinity);
        if (sched_setaffinity(0, sizeof(probedAffinity), &probedAffinity) != 0) {
            return false;
        }
        return CpuTopologyDetail::getCoreTypeOfCurrentCpu(outCoreType);
    };
    outMask = CpuTopologyDetail::probeCoreTypeAcrossCpus(processMask, coreType, probeSingleCpu, anyCoreTypeReported);

    if (sched_setaffinity(0, sizeof(originalAffinity), &originalAffinity) != 0) {
        errorMessage = std::string("failed to restore CPU affinity after probing core types, errno=") + std::strerror(errno);
        outMask = 0u;
        return CoreTypeLookupResult::NoMatchingCpu;
    }

    if (outMask == 0u) {
        return CpuTopologyDetail::classifyEmptyResult(coreType, anyCoreTypeReported, errorMessage);
    }
    return CoreTypeLookupResult::Success;
}
