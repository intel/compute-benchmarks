/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel void eat_time(int operationsCount) {
	volatile int value = 1u;
	for(int i =0;i<operationsCount;i++){
		value /=2;
		value *=2;
	}
}
