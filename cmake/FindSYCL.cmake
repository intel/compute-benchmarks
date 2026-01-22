#
# Copyright (C) 2022-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

set(SYCL_FOUND FALSE)

if(NOT SYCL_COMPILER_ROOT)
    set(SYCL_COMPILER_ROOT $ENV{CMPLR_ROOT})

    if(NOT SYCL_COMPILER_ROOT)
        get_filename_component(SYCL_ROOT "${CMAKE_SOURCE_DIR}/../oneapi" ABSOLUTE)
        if(IS_DIRECTORY "${SYCL_ROOT}")
            set(SYCL_COMPILER_ROOT "${SYCL_ROOT}")
            set(SYCL_LIB_PATH "${SYCL_COMPILER_ROOT}/lib")
            set(SYCL_BIN_PATH "${SYCL_COMPILER_ROOT}/bin")

            message(STATUS "FindSYCL: Using SYCL_COMPILER_ROOT from ${SYCL_COMPILER_ROOT}")
            message(STATUS "FindSYCL: SYCL_LIB_PATH = ${SYCL_LIB_PATH}")
            message(STATUS "FindSYCL: SYCL_BIN_PATH = ${SYCL_BIN_PATH}")

            list(APPEND CMAKE_PROGRAM_PATH "${SYCL_BIN_PATH}")
            set(ENV{LD_LIBRARY_PATH} "${SYCL_LIB_PATH}:$ENV{LD_LIBRARY_PATH}")
        endif()
    else()
        set(SYCL_COMPILER_ROOT ${SYCL_COMPILER_ROOT}/linux)
    endif()

    if(NOT SYCL_COMPILER_ROOT)
        message(WARNING "FindSYCL: Intel oneAPI environment not set. Ensure that Intel oneAPI is installed and use the setvars script.")
        return()
    endif()
endif()

if(NOT SYCL_COMPILER)
    if (WIN32)
        set(SYCL_COMPILER icx)
    else()
        find_program(HAS_ICPX "icpx" NO_CACHE)
        if (HAS_ICPX)
            set(SYCL_COMPILER icpx)
        else()
            set(SYCL_COMPILER clang++)
        endif()
    endif()
endif()

if(SYCL_COMPILER STREQUAL "acpp")
    set(SYCL_FLAGS "")
else()
    set(SYCL_FLAGS "-fsycl")
    if(BUILD_SYCL_WITH_CUDA)
        set(SYCL_FLAGS "${SYCL_FLAGS} -fsycl-targets=nvptx64-nvidia-cuda,spir64")
    endif()
endif()

set(SYCL_FOUND TRUE)
set(SYCL_CFLAGS "${SYCL_FLAGS}" CACHE STRING "SYCL Compiler Flags")
set(SYCL_LFLAGS "${SYCL_CFLAGS}" CACHE STRING "SYCL Linker Flags")
if(SYCL_COMPILER STREQUAL "acpp")
    set(SYCL_INCLUDE_DIR "${SYCL_COMPILER_ROOT}/include/AdaptiveCpp/" CACHE PATH "SYCL Include Directory")
else()
    set(SYCL_INCLUDE_DIR "${SYCL_COMPILER_ROOT}/include" CACHE PATH "SYCL Include Directory")
endif()
set(SYCL_LIBRARY_DIR "${SYCL_COMPILER_ROOT}/lib" CACHE PATH "SYCL Library Directory")

configure_file(${PROJECT_SOURCE_DIR}/scripts/sycl-proxy.sh.in "${CMAKE_CURRENT_BINARY_DIR}/sycl-proxy.sh" @ONLY)
set(SYCL_PROXY "${CMAKE_CURRENT_BINARY_DIR}/sycl-proxy.sh" CACHE STRING "SYCL Compiler Proxy")
