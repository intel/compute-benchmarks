/*
 * Copyright (C) 2022-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

typedef struct {
    int values[2];
} st_input_8;

kernel void arg_size(st_input_8 input) {
	volatile st_input_8 value;
	value = input;
}
