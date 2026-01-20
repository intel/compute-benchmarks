/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/argument/argument_container.h"

struct TestCaseArgumentContainer : ArgumentContainer {

    std::string getCurrentConfig(bool commandLine) const;
    bool validateArguments() const override;

    Api api = Api::Unknown;
    size_t iterations = 0;
    size_t warmupIterations = 0;
    bool noIntelExtensions = false;
    bool isSingleTestMode = false;
};
