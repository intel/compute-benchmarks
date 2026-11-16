#
# Copyright (C) 2025-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

add_benchmark(algorithm_benchmark ocl l0 all)
if(BUILD_L0)
    add_benchmark_dependency_on_workload(algorithm_benchmark heat3d_workload_l0 l0)
endif()
