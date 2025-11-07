/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/enum/api_additional.h"

#include "framework/enum/api.h"
#include "framework/utility/error.h"

#include <string>

std::string to_string_additional(Api api) {
    switch (api) {
    case Api::OPT:
        return "opt";
    default:
        FATAL_ERROR("Unknown API");
    }
}

std::string getUserFriendlyAdditionalApiName(Api api) {
    switch (api) {
    case Api::OPT:
        return "opt";
    default:
        FATAL_ERROR("Unknown API");
    }
}

Api parseAdditionalApi(const std::string &value) {
    if (value == "opt") {
        return Api::OPT;
    } else {
        return Api::Unknown;
    }
}
