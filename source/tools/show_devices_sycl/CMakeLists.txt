#
# Copyright (C) 2022 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

if (NOT BUILD_SYCL)
    return()
endif()

set(TARGET_NAME show_devices_sycl)
add_executable(${TARGET_NAME} CMakeLists.txt)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER tools)
add_sources_to_benchmark(${TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${TARGET_NAME} PRIVATE compute_benchmarks_framework_sycl)

# Additional config
setup_vs_folders(${TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR})
setup_output_directory(${TARGET_NAME})
