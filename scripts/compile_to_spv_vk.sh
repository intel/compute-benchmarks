#!/bin/bash

#
# Copyright (C) 2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

# example glslc cmdline:
# glslc --target-env=vulkan1.2 -o vk_ulls_benchmark_empty_kernel.spv vk/ulls_benchmark_empty_kernel.comp
if [[ $# -eq 1 ]]; then
  glslc --target-env=vulkan1.2 -o "vk_$(basename ${1%.*}).spv" $1
elif [[ $# -eq 2 ]]; then
  glslc --target-env=vulkan1.2 -o "vk_$(basename ${1%.*}).spv" $2 $1
else
  echo "usage:"
  echo "  $0 vulkan_shader.comp"
  echo "  $0 vulkan_shader.comp \"defines\""
  echo "examples:"
  echo "  $0 vk/ulls_benchmark_empty_kernel.comp"
  echo "  $0 vk/ulls_benchmark_empty_kernel.comp \"-DSOME_DEFINE=1\""
  echo "notes:"
  echo "  script requires glslc and required libraries to be in PATH"
  echo "  the vk_ prefix of the output is mandatory, kernels are copied flat into the output directory"
fi
