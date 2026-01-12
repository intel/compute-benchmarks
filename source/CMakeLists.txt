#
# Copyright (C) 2022-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

include(${BRANCH_TYPE}/CMakeUtilsAdditional.cmake OPTIONAL)
include(CMakeUtils.cmake)

set(SOURCE_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
add_subdirectory(framework)
add_subdirectory(workloads)
add_subdirectory(benchmarks)
add_subdirectory(tools)
add_subdirectory(docs_generator)
