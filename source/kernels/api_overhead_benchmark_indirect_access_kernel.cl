/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

typedef struct _st_container st_container;

struct _st_container
{
	__global int *value;
	__global st_container *next;
};

kernel void indirectAccess(__global st_container *container) {
	int value_to_write;
	for(value_to_write = 1; container->next; ++value_to_write, container=container->next){}
	*container->value = value_to_write;
}
