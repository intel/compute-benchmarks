/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/bitmap_argument.h"
#include "framework/enum/engine.h"

#include <string>
#include <vector>

using CcsBitmaskArgument = BitmaskArgument<EngineHelper::maxNumberOfComputeEngines, true>;
using BcsBitmaskArgument = BitmaskArgument<EngineHelper::maxNumberOfCopyEngines, true>;

struct SelectedEngine {
    Engine engine{};
    std::string label{};
    bool isCopyEngine{};
    bool isHighPriority{};
};

struct EngineBitmaskHelper {
    static std::vector<SelectedEngine> getSelectedEngines(const CcsBitmaskArgument &ccsMask, const BcsBitmaskArgument &bcsMask, const bool useHpCopyEngine) {
        std::vector<SelectedEngine> result{};
        for (const size_t index : ccsMask.getEnabledBits()) {
            const auto engine = EngineHelper::getComputeEngineFromIndex(index);
            result.push_back({engine, EngineHelper::getEngineName(engine), false, false});
        }
        for (const size_t index : bcsMask.getEnabledBits()) {
            const auto engine = EngineHelper::getBlitterEngineFromIndex(index);
            result.push_back({engine, EngineHelper::getEngineName(engine), true, false});
        }
        if (useHpCopyEngine) {
            result.push_back({Engine::Bcs, "BCS-HP", true, true});
        }
        return result;
    }
};
