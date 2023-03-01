#
# Copyright (C) 2022-2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

cmake_minimum_required(VERSION 3.8.0 FATAL_ERROR)
project(ComputeBenchmarks)
include(CMakeOptions.cmake)
set(PREDEFINED_TARGETS_FOLDER "CustomTargets")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")



# Enable MT Compilation in VisualStudio
if (MSVC)
    add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DELAYLOAD:OpenCL.dll")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DELAYLOAD:ze_loader.dll")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ignore:4199")
endif()

# Load dependencies
if(NOT EXISTS third_party/googletest/CMakeLists.txt)
    execute_process(COMMAND git submodule update --init --recursive
                    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endif()

# Produce version string
string(TIMESTAMP versionDate "%Y%m%d")
execute_process (COMMAND git log -1 --format=%h
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE versionCommit
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
set(BENCHMARK_VERSION "${versionDate}_${VERSION_INDEX}_${versionCommit}")

# Publish build additional handling
if(BUILD_FOR_PUBLISHING)
    # Only publish release builds
    message(STATUS "Overriding build type to Release")
    set(CMAKE_CONFIGURATION_TYPES Release)
    set(CMAKE_BUILD_TYPE Release)

    file(WRITE ${OUTPUT_DIR}/version.txt ${BENCHMARK_VERSION})
    message(STATUS "Version ${BENCHMARK_VERSION} written to version.txt")
endif()

# ccache
option(CCACHE_ALLOWED "allow use of ccache" TRUE)
find_program(CCACHE_EXE_FOUND ccache)
if(CCACHE_EXE_FOUND AND CCACHE_ALLOWED)
    message(STATUS "Found ccache: ${CCACHE_EXE_FOUND}")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif()

# Gtest
set(gtest_force_shared_crt OFF CACHE BOOL "")
set(BUILD_GMOCK OFF CACHE BOOL "")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "")
add_subdirectory(third_party/googletest)
if (MSVC)
    set_target_properties(gtest_main PROPERTIES FOLDER framework)
    set_target_properties(gtest PROPERTIES
        FOLDER framework
        LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    )
endif()

# Benchmark
if(MSVC)
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_DEBUG} /MT")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_DEBUG} /MT")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
endif()

# Compiler proxy for SYCL
if(BUILD_SYCL)
    find_package(SYCL)
    if(SYCL_FOUND)
        string(REPLACE "<CMAKE_CXX_COMPILER>" "${SYCL_PROXY} <CMAKE_CXX_COMPILER>" CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${SYCL_PROXY}")
    endif()
endif()

if(BUILD_OMP)
    set(CMAKE_CXX_COMPILER "icpx")
endif()

add_subdirectory(source)
set_directory_properties(PROPERTIES VS_STARTUP_PROJECT ulls_benchmark_ocl)
