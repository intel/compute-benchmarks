/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "cpu_allocation_helper.h"

#include <cstdint>

CpuAllocationHelper::AlignedAllocation::AlignedAllocation(std::unique_ptr<std::byte[]> &&rawMemory, std::byte *alignedMemory)
    : rawMemory(std::move(rawMemory)),
      alignedMemory(alignedMemory) {}

CpuAllocationHelper::MisalignedAllocation::MisalignedAllocation(AlignedAllocation &&alignedAllocation, std::byte *misalignedMemory)
    : alignedAllocation(std::move(alignedAllocation)),
      misalignedMemory(misalignedMemory) {}

CpuAllocationHelper::AlignedAllocation CpuAllocationHelper::allocateAlignedAllocation(size_t size, size_t alignment) {
    auto rawMemory = std::make_unique<std::byte[]>(size + alignment);
    auto misalignment = reinterpret_cast<uintptr_t>(rawMemory.get()) % alignment;
    auto alignedMemory = rawMemory.get() + alignment - misalignment;
    return AlignedAllocation(std::move(rawMemory), alignedMemory);
}

CpuAllocationHelper::MisalignedAllocation CpuAllocationHelper::allocateMisalignedAllocation(size_t size, size_t alignment, size_t misalignment) {
    AlignedAllocation alignedAllocation = allocateAlignedAllocation(size, alignment);
    std::byte *misalignedMemory = alignedAllocation + misalignment;
    return MisalignedAllocation(std::move(alignedAllocation), misalignedMemory);
}
