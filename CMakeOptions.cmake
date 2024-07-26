#
# Copyright (C) 2022-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

macro(benchmark_option NAME DEFAULT_VALUE)
    if (NOT DEFINED ${NAME})
        set(${NAME} ${DEFAULT_VALUE})
    endif()
    if (${NAME} STREQUAL "")
        set(${NAME} ${DEFAULT_VALUE})
    endif()
    message(STATUS "      ${NAME}=${${NAME}} ${ARGN}")
endmacro()
macro (benchmark_option_group DESCRIPTION)
    message(STATUS "   ${DESCRIPTION}:")
endmacro()
message(STATUS "Build options:")

# Selecting targets
benchmark_option_group("Flags for selecting targets to be built")
benchmark_option(BUILD_L0 ON)
benchmark_option(BUILD_OCL ON)
benchmark_option(BUILD_UR OFF)
benchmark_option(BUILD_SYCL OFF)
benchmark_option(BUILD_SYCL_WITH_CUDA OFF)
benchmark_option(BUILD_OMP OFF)
benchmark_option(BUILD_MPI OFF)
if(BUILD_SYCL AND MSVC)
    set(BUILD_SYCL OFF)
    message(WARNING "Building SYCL benchmarks is disabled on Windows because of incompatibility of the dynamically-linked Visual C++ Runtime, required for DPC++, with GoogleTest")
endif()
if (NOT BUILD_L0 OR NOT BUILD_OCL)
    set(BUILD_ALL_API_BINARIES OFF)
    set(BUILD_ALL_API_BINARIES_COMMENT "(Disabled due to not all APIs being enabled)")
endif()
if (BUILD_SYCL)
    set(BUILD_ALL_API_BINARIES OFF)
    set(BUILD_ALL_API_BINARIES_COMMENT "(Disabled due to SYCL being enabled)")
endif()
benchmark_option(BUILD_SINGLE_API_BINARIES ON)
benchmark_option(BUILD_ALL_API_BINARIES OFF ${BUILD_ALL_API_BINARIES_COMMENT})

# Publish builds and versioning
benchmark_option_group("Flags for configuring publishable builds and versions")
benchmark_option(BUILD_FOR_PUBLISHING OFF)
if (BUILD_FOR_PUBLISHING)
    set(INCLUDE_VERSION_COMMENT "(Enabled by BUILD_FOR_PUBLISHING)")
endif()
benchmark_option(INCLUDE_VERSION OFF ${INCLUDE_VERSION_COMMENT})
benchmark_option(VERSION_INDEX "01")

# Miscellaneous flags
benchmark_option_group("Miscellaneous flags")
benchmark_option(OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin")
benchmark_option(BUILD_HELLO_WORLD OFF)
benchmark_option(GENERATE_DOCS ON)
benchmark_option(BUILD_TOOLS ON)
benchmark_option(LOG_BENCHMARK_TARGETS OFF)
benchmark_option(ALLOW_WARNINGS OFF)

# Additional checks
if (NOT BUILD_L0 AND NOT BUILD_OCL AND NOT BUILD_SYCL AND NOT BUILD_UR)
    message(FATAL_ERROR "No API was selected for testing. No benchmarks will be produced")
endif()
