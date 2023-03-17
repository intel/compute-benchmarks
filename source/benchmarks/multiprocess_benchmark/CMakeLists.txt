#
# Copyright (C) 2022-2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

add_benchmark(multiprocess_benchmark ocl l0 all)
add_benchmark_dependency_on_workload(multiprocess_benchmark single_queue_workload_l0 l0)
add_benchmark_dependency_on_workload(multiprocess_benchmark single_queue_workload_shared_buffer_l0 l0)
