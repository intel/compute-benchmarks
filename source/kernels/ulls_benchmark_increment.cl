/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel __attribute__((reqd_work_group_size(1, 1, 1))) void increment(global int *value) {
    (*value)++;
}
