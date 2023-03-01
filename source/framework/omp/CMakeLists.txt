#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

if (NOT BUILD_OMP)
    return()
endif()

# Define target
set(API_NAME omp)
set(TARGET_NAME compute_benchmarks_framework_${API_NAME})
add_library(${TARGET_NAME} STATIC ${SOURCES})
target_link_libraries(${TARGET_NAME} PUBLIC compute_benchmarks_framework -qopenmp -fopenmp-targets=spir64)
target_compile_options(${TARGET_NAME} PUBLIC -qopenmp -fopenmp-targets=spir64)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER framework)
add_subdirectories()
setup_vs_folders(${TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR})
setup_warning_options(${TARGET_NAME})
setup_output_directory(${TARGET_NAME})

# Add this API to global array
set_property(GLOBAL APPEND PROPERTY APIS ${API_NAME})
