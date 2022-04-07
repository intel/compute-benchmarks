/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

struct ExecuteAtAppInit {
    template <typename Code>
    ExecuteAtAppInit(Code code) {
        code();
    }
};

#define EXECUTE_AT_APP_INIT_WITH_ID(identifier) \
    static ExecuteAtAppInit EXECUTE_AT_APP_INIT_##identifier##_object = []()

#define EXECUTE_AT_APP_INIT EXECUTE_AT_APP_INIT_WITH_ID(default)
