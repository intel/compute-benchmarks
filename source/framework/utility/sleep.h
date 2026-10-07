/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#ifdef WIN32
#include "framework/utility/windows/sleep_windows.h"
#else // !WIN32
#include "framework/utility/linux/sleep_linux.h"
#endif // !WIN32
