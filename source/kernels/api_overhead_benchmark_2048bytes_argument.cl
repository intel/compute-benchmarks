/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

typedef struct {
    int values[512];
} st_input_2048;

kernel void arg_size(st_input_2048 input) {
	volatile st_input_2048 value;
	value = input;
}
