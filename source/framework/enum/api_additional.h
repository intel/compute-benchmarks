/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <string>

enum class Api;

std::string to_string_additional(Api api);

std::string getUserFriendlyAdditionalApiName(Api api);

Api parseAdditionalApi(const std::string &value);
