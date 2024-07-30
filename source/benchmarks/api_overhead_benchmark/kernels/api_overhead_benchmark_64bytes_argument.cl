/*
 * Copyright (C) 2022-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

typedef struct {
    int values[16];
} st_input_64;

kernel void arg_size(st_input_64 input) {
	volatile st_input_64 value;
	value = input;
}
