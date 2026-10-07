/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/argument_container.h"

struct AdditionalConfiguration : ArgumentContainer {
  public:
    AdditionalConfiguration(ArgumentContainer &parent);
};
