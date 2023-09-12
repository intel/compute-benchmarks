#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

if (MPI_FOUND)
    add_benchmark(mpi_benchmark l0 all)
    add_benchmark_dependency_on_workload(mpi_benchmark mpi_workload_l0 l0)
endif()
