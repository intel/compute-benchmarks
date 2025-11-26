/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifdef WIN32
#include "framework/utility/windows/sleep_windows.h"
#else // !WIN32
#include "framework/utility/linux/sleep_linux.h"
#endif // !WIN32
