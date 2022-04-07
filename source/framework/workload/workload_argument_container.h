/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/argument_container.h"
#include "framework/argument/basic_argument.h"

struct WorkloadArgumentContainer : ArgumentContainer {
    PositiveIntegerArgument iterations;
    BooleanArgument synchronize;
    IntegerArgument synchronizationPipeIn;
    IntegerArgument synchronizationPipeOut;
    IntegerArgument measurementPipe;

    WorkloadArgumentContainer()
        : iterations(*this, "iterations", "Number of iterations to perform"),
          synchronize(*this, "synchronize", "Wait for synchronization before each iteration"),
          synchronizationPipeIn(*this, "synchronizationPipeIn", "Handle for the synchronization pipe (parent to child). If 0, stdin is used."),
          synchronizationPipeOut(*this, "synchronizationPipeOut", "Handle for the synchronization pipe (child to parent). If 0, stdout is used."),
          measurementPipe(*this, "measurementPipe", "Handle for the measurements pipe. If 0, stdout is used") {

        // Default values
        iterations = 10;

        // Default values for parameters used by benchmarks calling the workload. Not meant to be used by humans.
        synchronize = false;
        synchronizationPipeIn = 0;
        synchronizationPipeOut = 0;
        measurementPipe = 0;
    }
};
