/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/enum_argument.h"
#include "framework/enum/graph_structure.h"

struct GraphStructureArgument : EnumArgument<GraphStructureArgument, GraphStructure> {
    using EnumArgument::EnumArgument;
    ThisType &operator=(EnumType newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    const static inline std::string enumName = "graph structure";
    const static inline EnumType invalidEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[3] = {EnumType::Gromacs, EnumType::LLama, EnumType::Amr};
    const static inline std::string enumValuesNames[3] = {"Gromacs", "LLama", "Amr"};
};
