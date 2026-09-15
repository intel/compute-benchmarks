#!/bin/bash

#
# Copyright (C) 2024-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

# example ocloc cmdline:
# ocloc -file kernel.cl -internal_options "-DATTRIBUTE=123" -spv_only -output_no_suffix
if [[ $# -eq 1 ]]; then
  ocloc -file $1 -spv_only -output_no_suffix
elif [[ $# -eq 2 ]]; then
  ocloc -file $1 -spv_only -output_no_suffix -internal_options "$2"
else
  echo "usage:"
  echo "  $0 opencl_kernel.cl"
  echo "  $0 opencl_kernel.cl \"defines\""
  echo "examples:"
  echo "  $0 api_overhead_benchmark_eat_time.cl"
  echo "notes:"
  echo "  script requires ocloc and required libraries to be in PATH"
fi