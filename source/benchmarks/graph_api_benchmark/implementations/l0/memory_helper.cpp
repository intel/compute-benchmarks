/*
 * Copyright (C) 2024-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "implementations/l0/memory_helper.h"

namespace MemHelper {
DataFloatPtr alloc(UsmMemoryPlacement placement, std::shared_ptr<LevelZero> levelzero, uint32_t count) {
    void *deviceptr = nullptr;
    EXPECT_ZE_RESULT_SUCCESS(L0::UsmHelper::allocate(placement, *levelzero, count * sizeof(float), &deviceptr));

    return DataFloatPtr(static_cast<float *>(deviceptr), [placement, levelzero](float *ptr) {
        EXPECT_ZE_RESULT_SUCCESS(L0::UsmHelper::deallocate(placement, *levelzero, ptr));
    });
}
} // namespace MemHelper
