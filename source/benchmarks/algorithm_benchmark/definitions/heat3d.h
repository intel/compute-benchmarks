/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct Heat3DArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument subDomainX;
    PositiveIntegerArgument subDomainY;
    PositiveIntegerArgument subDomainZ;
    PositiveIntegerArgument timesteps;
    PositiveIntegerArgument meshLength;

    Heat3DArguments()
        : subDomainX(*this, "subDomainX", "Number of sub-domains in the X-direction"),
          subDomainY(*this, "subDomainY", "Number of sub-domains in the Y-direction"),
          subDomainZ(*this, "subDomainZ", "Number of sub-domains in the Z-direction"),
          timesteps(*this, "timesteps", "Number of simulation timesteps"),
          meshLength(*this, "meshLength", "Number of mesh points along each of the X-Y-Z directions") {
        subDomainX = 1;
        subDomainY = 1;
        subDomainZ = 1;
        timesteps = 100;
        meshLength = 64;
    }
};

struct Heat3D : TestCase<Heat3DArguments> {
    using TestCase<Heat3DArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "Heat3D";
    }

    std::string getHelp() const override {
        return "A 3D heat-equation solving benchmark that overlaps IPC data transfers (nearest-neighbor halo exchange) and GPU compute kernels."
               "Measures multi-process concurrent kernel execution and IPC memory transfer performance on a single device.";
    }
};
