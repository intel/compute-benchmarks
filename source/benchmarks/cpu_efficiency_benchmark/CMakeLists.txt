#
# Copyright (C) 2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

add_benchmark(cpu_efficiency_benchmark l0)

if(TARGET cpu_efficiency_benchmark_l0)
    if(WIN32)
        target_sources(cpu_efficiency_benchmark_l0 PRIVATE implementations/l0/windows/event_host_synchronize_l0.cpp)
    else()
        target_sources(cpu_efficiency_benchmark_l0 PRIVATE implementations/l0/linux/event_host_synchronize_l0.cpp)
    endif()
endif()

add_subdirectories()
