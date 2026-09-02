/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/utility/cpu_topology.h"
#include "framework/utility/windows/windows.h"

#include <intrin.h>
#include <string>

namespace CpuTopologyDetail {
namespace {
constexpr size_t cpuidRegisterCount = 4u;
} // namespace

CpuidRegisters cpuidEx(uint32_t leaf, uint32_t subLeaf) {
    int rawRegisters[cpuidRegisterCount]{};
    __cpuidex(rawRegisters, static_cast<int>(leaf), static_cast<int>(subLeaf));
    CpuidRegisters registers{};
    registers.eax = static_cast<uint32_t>(rawRegisters[0]);
    registers.ebx = static_cast<uint32_t>(rawRegisters[1]);
    registers.ecx = static_cast<uint32_t>(rawRegisters[2]);
    registers.edx = static_cast<uint32_t>(rawRegisters[3]);
    return registers;
}
} // namespace CpuTopologyDetail

CoreTypeLookupResult getCpuMaskForCoreType(CpuCoreType coreType, uint64_t &outMask, std::string &errorMessage) {
    outMask = 0u;
    if (!CpuTopologyDetail::isIntelHybridCpu(errorMessage)) {
        return CoreTypeLookupResult::NotHybrid;
    }

    // Relative to the process's current processor group, like SetProcessAffinityMask in the pinning path.
    DWORD_PTR processAffinity = 0u;
    DWORD_PTR systemAffinity = 0u;
    if (GetProcessAffinityMask(GetCurrentProcess(), &processAffinity, &systemAffinity) == 0) {
        errorMessage = "GetProcessAffinityMask failed: " + getErrorFromLastErrorCode();
        return CoreTypeLookupResult::NoMatchingCpu;
    }

    const HANDLE currentThread = GetCurrentThread();
    DWORD_PTR originalThreadAffinity = 0u;
    bool anyCoreTypeReported = false;
    const auto probeSingleCpu = [&](uint32_t cpu, CpuCoreType &outCoreType) {
        const DWORD_PTR previousAffinity = SetThreadAffinityMask(currentThread, static_cast<DWORD_PTR>(1ull << cpu));
        if (previousAffinity == 0u) {
            return false;
        }
        if (originalThreadAffinity == 0u) {
            originalThreadAffinity = previousAffinity;
        }

        // SetThreadAffinityMask does not guarantee the thread has moved when it returns, and Sleep(0) only yields
        // if another equal-priority thread is ready, so confirm the CPU before trusting CPUID.
        Sleep(0);
        if (GetCurrentProcessorNumber() != cpu) {
            return false;
        }
        return CpuTopologyDetail::getCoreTypeOfCurrentCpu(outCoreType);
    };
    outMask = CpuTopologyDetail::probeCoreTypeAcrossCpus(static_cast<uint64_t>(processAffinity), coreType, probeSingleCpu, anyCoreTypeReported);

    if (originalThreadAffinity != 0u && SetThreadAffinityMask(currentThread, originalThreadAffinity) == 0u) {
        errorMessage = "failed to restore thread affinity after probing core types: " + getErrorFromLastErrorCode();
        outMask = 0u;
        return CoreTypeLookupResult::NoMatchingCpu;
    }

    if (outMask == 0u) {
        return CpuTopologyDetail::classifyEmptyResult(coreType, anyCoreTypeReported, errorMessage);
    }
    return CoreTypeLookupResult::Success;
}
