/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/utility/cpu_topology.h"

std::string toString(CpuCoreType coreType) {
    return coreType == CpuCoreType::Performance ? "P-core" : "E-core";
}

namespace CpuTopologyDetail {

namespace {
constexpr uint32_t vendorLeaf = 0x0u;
constexpr uint32_t extendedFeaturesLeaf = 0x7u;
constexpr uint32_t hybridInformationLeaf = 0x1Au;

// CPUID.0H returns "GenuineIntel" split across EBX, EDX, ECX.
constexpr uint32_t intelVendorEbx = 0x756e6547u;
constexpr uint32_t intelVendorEdx = 0x49656e69u;
constexpr uint32_t intelVendorEcx = 0x6c65746eu;

// CPUID.07H.0:EDX[15] is the hybrid flag. Leaf 0x1A alone does not imply a P/E split, because P-only and E-only
// parts implement it too.
constexpr uint32_t hybridFeatureBit = 1u << 15;

// CPUID.1AH.0:EAX[31:24] is an enumerated core type (Linux X86_CPU_TYPE_INTEL_CORE/ATOM), compared for equality,
// not masked. It can read 0 on a hybrid part, for example Alder Lake SKUs that ship only P-cores.
constexpr uint32_t coreTypeShift = 24u;
constexpr uint32_t coreTypeAtom = 0x20u;
constexpr uint32_t coreTypeCore = 0x40u;
} // namespace

bool isIntelHybridCpu(std::string &errorMessage) {
    const CpuidRegisters vendor = cpuidEx(vendorLeaf, 0u);
    if (vendor.ebx != intelVendorEbx || vendor.edx != intelVendorEdx || vendor.ecx != intelVendorEcx) {
        errorMessage = "core-type filtering needs an Intel CPU, but CPUID reports a different vendor";
        return false;
    }
    if (vendor.eax < hybridInformationLeaf) {
        errorMessage = "this CPU does not implement CPUID leaf 0x1A, so it reports no core types (not a hybrid part)";
        return false;
    }

    const CpuidRegisters extendedFeatures = cpuidEx(extendedFeaturesLeaf, 0u);
    if ((extendedFeatures.edx & hybridFeatureBit) == 0u) {
        errorMessage = "this CPU is not a hybrid part (CPUID.07H:EDX[15] is clear), so it has no P-core/E-core split";
        return false;
    }
    return true;
}

bool getCoreTypeOfCurrentCpu(CpuCoreType &outCoreType) {
    const CpuidRegisters hybridInformation = cpuidEx(hybridInformationLeaf, 0u);
    switch (hybridInformation.eax >> coreTypeShift) {
    case coreTypeCore:
        outCoreType = CpuCoreType::Performance;
        return true;
    case coreTypeAtom:
        outCoreType = CpuCoreType::Efficiency;
        return true;
    default:
        return false;
    }
}

uint64_t probeCoreTypeAcrossCpus(uint64_t processMask, CpuCoreType coreType,
                                 const std::function<bool(uint32_t cpu, CpuCoreType &outCoreType)> &probeSingleCpu,
                                 bool &anyCoreTypeReported) {
    uint64_t matchingCpus = 0u;
    anyCoreTypeReported = false;

    for (uint32_t cpu = 0u; cpu < maxDetectableCpuCount; ++cpu) {
        if ((processMask & (1ull << cpu)) == 0u) {
            continue;
        }

        CpuCoreType probedCoreType{};
        if (probeSingleCpu(cpu, probedCoreType)) {
            anyCoreTypeReported = true;
            if (probedCoreType == coreType) {
                matchingCpus |= 1ull << cpu;
            }
        }
    }
    return matchingCpus;
}

CoreTypeLookupResult classifyEmptyResult(CpuCoreType coreType, bool anyCoreTypeReported, std::string &errorMessage) {
    if (!anyCoreTypeReported) {
        errorMessage = "this CPU sets the hybrid flag but CPUID leaf 0x1A reported no core type for any "
                       "available CPU (seen on Alder Lake SKUs that ship only P-cores), so core types "
                       "cannot be told apart here";
        return CoreTypeLookupResult::NotHybrid;
    }
    errorMessage = "no " + toString(coreType) + " is available to this process";
    return CoreTypeLookupResult::NoMatchingCpu;
}

} // namespace CpuTopologyDetail
