#
# Copyright (C) 2022 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

include(CMakeUtils.cmake)

set(SOURCE_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
add_subdirectory(framework)
add_subdirectory(workloads)
add_subdirectory(benchmarks)
add_subdirectory(tools)
add_subdirectory(docs_generator)
