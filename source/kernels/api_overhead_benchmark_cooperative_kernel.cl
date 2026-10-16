/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel void cooperative_kernel(global int *buffer) {
  const int work_group_id = get_group_id(0);
  
  buffer[work_group_id] = work_group_id;

  intel_device_barrier(CLK_GLOBAL_MEM_FENCE);

  for (int i = 0; i < get_num_groups(0); ++i) {
    if (buffer[i] != i) {
      buffer[work_group_id] = -1;
      break;
    }
  }
}