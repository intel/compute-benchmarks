/*
 * Copyright (C) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel void empty() {
}

kernel void emptyWithSynchro(unsigned long eventAddressValue, unsigned long waitValue, unsigned int delay){
if(eventAddressValue){
  if(get_local_id(0)==0)
  {
      __global unsigned int* eventAddress = (__global unsigned int*) eventAddressValue;
      while(atomic_or(eventAddress,0) != waitValue) {
      if(delay>1) {
        //do some pause to make sure we do not bombard chip with atomic reads
        volatile int counter = delay;
        while (counter--);
        }
      }
  }
  barrier(CLK_GLOBAL_MEM_FENCE);
}
}