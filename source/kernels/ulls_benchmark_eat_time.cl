/*
 * Copyright (C) 2022-2026 Intel Corporation
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

kernel void eat_time2(int operationsCount) {
	volatile int value = 2u;
	for(int i =0;i<operationsCount;i++){
		value += 3;
		value -= 3;
	}
}
