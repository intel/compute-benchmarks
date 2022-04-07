/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/enum/api.h"

class SupportedApis {
  public:
    static void registerSupportedApi(Api api);
    static bool isApiSupported(Api api);

  private:
    static inline bool supportedApis[static_cast<int>(Api::COUNT)] = {};
};
