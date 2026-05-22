#
# Copyright (C) 2022-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

if(SYCL_COMPILER STREQUAL "acpp")
    add_benchmark(graph_api_benchmark l0 ur ocl all)
else()
    add_benchmark(graph_api_benchmark sycl l0 ur syclpreview ocl all)
    if(TARGET graph_api_benchmark_sycl AND TARGET compute_benchmarks_framework_l0)
        target_link_libraries(graph_api_benchmark_sycl PRIVATE compute_benchmarks_framework_l0)
    endif()
    if(TARGET graph_api_benchmark_syclpreview AND TARGET compute_benchmarks_framework_l0)
        target_link_libraries(graph_api_benchmark_syclpreview PRIVATE compute_benchmarks_framework_l0)
    endif()
endif()
