/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <cstddef>
#include <memory>

struct CpuAllocationHelper {
    class AlignedAllocation {
      public:
        AlignedAllocation(std::unique_ptr<std::byte[]> &&rawMemory, std::byte *alignedMemory);
        std::byte *get() { return alignedMemory; }
        operator std::byte *() { return alignedMemory; }

      private:
        std::unique_ptr<std::byte[]> rawMemory;
        std::byte *alignedMemory;
    };

    class MisalignedAllocation {
      public:
        MisalignedAllocation(AlignedAllocation &&alignedAllocation, std::byte *misalignedMemory);
        std::byte *get() { return misalignedMemory; }
        operator std::byte *() { return misalignedMemory; }

      private:
        AlignedAllocation alignedAllocation;
        std::byte *misalignedMemory;
    };

    static AlignedAllocation allocateAlignedAllocation(size_t size, size_t alignment);
    static MisalignedAllocation allocateMisalignedAllocation(size_t size, size_t alignment, size_t misalignment);
};
