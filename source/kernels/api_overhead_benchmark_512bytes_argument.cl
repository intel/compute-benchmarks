/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

typedef struct {
    int values[128];
} st_input_512;

kernel void arg_size(st_input_512 input) {
	volatile st_input_512 value;
	value = input;
}
