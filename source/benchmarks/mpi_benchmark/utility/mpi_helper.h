/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <chrono>
#include <string>
#include <vector>

std::vector<std::chrono::nanoseconds> runLauncher(const std::string &launcher, const int nRanks, const std::string &binary, const std::string &args, const int nIters);
