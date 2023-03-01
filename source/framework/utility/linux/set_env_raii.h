/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <stdlib.h>

class SetEnvRAII {
  public:
    SetEnvRAII(const char *env, const char *value) : name(env) {
        setenv(this->name, value, 1);
    }
    ~SetEnvRAII() {
        unsetenv(this->name);
    }

  private:
    const char *name = nullptr;
};