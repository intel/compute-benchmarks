/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <cstdint>
#include <functional>
#include <string>

enum class CpuCoreType {
    Performance,
    Efficiency,
};

std::string toString(CpuCoreType coreType);

// NotHybrid leaves CPU affinity untouched; NoMatchingCpu is an argument error.
enum class CoreTypeLookupResult {
    Success,
    NotHybrid,
    NoMatchingCpu,
};

// Briefly pins the calling thread to every CPU this process may run on to read its core type, then restores
// the original affinity.
CoreTypeLookupResult getCpuMaskForCoreType(CpuCoreType coreType, uint64_t &outMask, std::string &errorMessage);

namespace CpuTopologyDetail {

constexpr uint32_t maxDetectableCpuCount = 64u;

struct CpuidRegisters {
    uint32_t eax = 0u;
    uint32_t ebx = 0u;
    uint32_t ecx = 0u;
    uint32_t edx = 0u;
};

CpuidRegisters cpuidEx(uint32_t leaf, uint32_t subLeaf);
bool isIntelHybridCpu(std::string &errorMessage);
bool getCoreTypeOfCurrentCpu(CpuCoreType &outCoreType);
CoreTypeLookupResult classifyEmptyResult(CpuCoreType coreType, bool anyCoreTypeReported, std::string &errorMessage);
uint64_t probeCoreTypeAcrossCpus(uint64_t processMask, CpuCoreType coreType,
                                 const std::function<bool(uint32_t cpu, CpuCoreType &outCoreType)> &probeSingleCpu,
                                 bool &anyCoreTypeReported);

} // namespace CpuTopologyDetail
