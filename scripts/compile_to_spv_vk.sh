#!/bin/bash

#
# Copyright (C) 2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

# example glslc cmdline:
# glslc --target-env=vulkan1.2 -o vk_ulls_benchmark_empty_kernel.spv vk/ulls_benchmark_empty_kernel.comp
if [[ $# -lt 1 || $# -gt 3 ]]; then
  echo "usage:"
  echo "  $0 vulkan_shader.comp"
  echo "  $0 vulkan_shader.comp \"defines\""
  echo "  $0 vulkan_shader.comp \"defines\" output_suffix"
  echo "examples:"
  echo "  $0 vk/ulls_benchmark_empty_kernel.comp"
  echo "  $0 vk/ulls_benchmark_empty_kernel.comp \"-DSOME_DEFINE=1\""
  echo "  $0 vk/api_overhead_benchmark_multi_arg_kernel.comp \"-DARG_COUNT=16 -DUSE_GLOBAL_IDS=0\" _16"
  echo "  $0 vk/api_overhead_benchmark_multi_arg_kernel.comp \"-DARG_COUNT=16 -DUSE_GLOBAL_IDS=1\" _16_ids"
  echo "notes:"
  echo "  script requires glslc and required libraries to be in PATH"
  echo "  the vk_ prefix of the output is mandatory, kernels are copied flat into the output directory"
  echo "  output_suffix distinguishes the binaries when one shader source is compiled several times"
  exit 1
fi

glslc --target-env=vulkan1.2 -o "vk_$(basename ${1%.*})$3.spv" $2 $1
