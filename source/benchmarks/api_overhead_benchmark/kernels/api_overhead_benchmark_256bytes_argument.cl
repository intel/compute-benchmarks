/*
 * Copyright (C) 2022-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

typedef struct {
    int values[64];
} st_input_256;

kernel void arg_size(st_input_256 input) {
	volatile st_input_256 value;
	value = input;
}
