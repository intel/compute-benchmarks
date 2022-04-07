/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/workload/workload.h"

template <typename Workload>
struct RegisterWorkloadImplementation {
    explicit RegisterWorkloadImplementation(typename Workload::WorkloadImplementation function) {
        Workload::implementation = function;
    }
};
