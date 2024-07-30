/*
 * Copyright (C) 2022-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

typedef struct {
    int values[256];
} st_input_1024;

kernel void arg_size(st_input_1024 input) {
	volatile st_input_1024 value;
	value = input;
}
