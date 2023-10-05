#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

add_benchmark(algorithm_benchmark l0 all)
add_benchmark_dependency_on_workload(algorithm_benchmark heat3d_workload_l0 l0)
