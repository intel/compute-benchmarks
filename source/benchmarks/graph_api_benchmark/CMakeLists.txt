#
# Copyright (C) 2022-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

if(SYCL_COMPILER STREQUAL "acpp")
    add_benchmark(graph_api_benchmark l0 ur ocl all)
else()
    add_benchmark(graph_api_benchmark sycl l0 ur syclpreview ocl all)
endif()