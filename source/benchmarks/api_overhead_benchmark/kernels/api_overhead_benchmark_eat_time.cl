/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel void eat_time(int operationsCount) {
    if(operationsCount > 4) {
        volatile int counter = operationsCount;
        while(--counter) ;
    }
}
