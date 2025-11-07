/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/argument/argument_container.h"

struct AdditionalConfiguration : ArgumentContainer {
  public:
    AdditionalConfiguration(ArgumentContainer &parent);
};
