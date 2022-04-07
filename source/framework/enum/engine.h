/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/utility/error.h"

#include <string>

enum class Engine {
    Unknown,

    // RenderCompute group
    Rcs,

    // Compute group
    Ccs0,
    Ccs1,
    Ccs2,
    Ccs3,

    // Copy group
    Bcs,

    // LinkCopy group
    Bcs1,
    Bcs2,
    Bcs3,
    Bcs4,
    Bcs5,
    Bcs6,
    Bcs7,
    Bcs8,
};

enum class EngineGroup {
    Unknown,
    RenderCompute,
    Compute,
    Copy,
    LinkCopy,
};

struct EngineHelper {
    static EngineGroup parseEngineGroup(const std::string &name) {
        if (name == "rcs" || name == "cccs") {
            return EngineGroup::RenderCompute;
        }
        if (name == "ccs") {
            return EngineGroup::Compute;
        }
        if (name == "bcs") {
            return EngineGroup::Copy;
        }
        if (name == "linked bcs") {
            return EngineGroup::LinkCopy;
        }
        return EngineGroup::Unknown;
    }

    static EngineGroup getEngineGroup(Engine engine) {
        switch (engine) {
        case Engine::Rcs:
            return EngineGroup::RenderCompute;
        case Engine::Ccs0:
        case Engine::Ccs1:
        case Engine::Ccs2:
        case Engine::Ccs3:
            return EngineGroup::Compute;
        case Engine::Bcs:
            return EngineGroup::Copy;
        case Engine::Bcs1:
        case Engine::Bcs2:
        case Engine::Bcs3:
        case Engine::Bcs4:
        case Engine::Bcs5:
        case Engine::Bcs6:
        case Engine::Bcs7:
        case Engine::Bcs8:
            return EngineGroup::LinkCopy;
        default:
            FATAL_ERROR("Unknown engine");
        }
    }

    static size_t getEngineIndexWithinGroup(Engine engine) {
        switch (engine) {
        case Engine::Rcs:
            return 0;
        case Engine::Ccs0:
        case Engine::Ccs1:
        case Engine::Ccs2:
        case Engine::Ccs3:
            return static_cast<size_t>(engine) - static_cast<size_t>(Engine::Ccs0);
        case Engine::Bcs:
            return 0;
        case Engine::Bcs1:
        case Engine::Bcs2:
        case Engine::Bcs3:
        case Engine::Bcs4:
        case Engine::Bcs5:
        case Engine::Bcs6:
        case Engine::Bcs7:
        case Engine::Bcs8:
            return static_cast<size_t>(engine) - static_cast<size_t>(Engine::Bcs1);
        default:
            FATAL_ERROR("Unknown engine");
        }
    }

    static Engine getBlitterEngineFromIndex(size_t index) {
        if (index == 0) {
            return Engine::Bcs;
        }

        if (index <= 8) {
            const size_t linkBcsBase = static_cast<size_t>(Engine::Bcs1);
            return static_cast<Engine>(linkBcsBase + index - 1);
        }

        FATAL_ERROR("Invalid blitter index");
    }

    static std::string getEngineName(Engine engine) {
        switch (engine) {
        case Engine::Rcs:
            return "RCS";
        case Engine::Ccs0:
            return "CCS0";
        case Engine::Ccs1:
            return "CCS1";
        case Engine::Ccs2:
            return "CCS2";
        case Engine::Ccs3:
            return "CCS3";
        case Engine::Bcs:
            return "BCS";
        case Engine::Bcs1:
            return "BCS1";
        case Engine::Bcs2:
            return "BCS2";
        case Engine::Bcs3:
            return "BCS3";
        case Engine::Bcs4:
            return "BCS4";
        case Engine::Bcs5:
            return "BCS5";
        case Engine::Bcs6:
            return "BCS6";
        case Engine::Bcs7:
            return "BCS7";
        case Engine::Bcs8:
            return "BCS8";
        default:
            FATAL_ERROR("Unknown engine");
        }
    }
};
