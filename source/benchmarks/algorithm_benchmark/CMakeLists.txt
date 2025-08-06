#
# Copyright (C) 2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

if(BUILD_L0)
    add_benchmark(algorithm_benchmark l0 all)
    add_benchmark_dependency_on_workload(algorithm_benchmark heat3d_workload_l0 l0)
endif()
