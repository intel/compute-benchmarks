#
# Copyright (C) 2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

if (NOT BUILD_UR)
    return()
endif()

find_package(unified-runtime)

if(${unified-runtime_FOUND} AND NOT BENCHMARK_UR_OVERRIDE_FETCH_CONTENT_REPO AND NOT BENCHMARK_UR_OVERRIDE_FETCH_CONTENT_TAG)

    message(STATUS "Unified Runtime installation found")

    add_library(ur_loader ALIAS unified-runtime::ur_loader)
    add_library(ur_headers ALIAS unified-runtime::ur_headers)

else()

    include(FetchContent)

    set(UMF_DISABLE_HWLOC ON CACHE INTERNAL "")
    set(UMF_BUILD_EXAMPLES OFF CACHE INTERNAL "EXAMPLES")
    set(UR_BUILD_TESTS OFF CACHE INTERNAL "TESTS")

    set(UNIFIED_RUNTIME_REPO "https://github.com/oneapi-src/unified-runtime.git")
    set(UNIFIED_RUNTIME_TAG main)

    if(BENCHMARK_UR_OVERRIDE_FETCH_CONTENT_REPO)
        set(UNIFIED_RUNTIME_REPO "${BENCHMARK_UR_OVERRIDE_FETCH_CONTENT_REPO}")
    endif()
    if(BENCHMARK_UR_OVERRIDE_FETCH_CONTENT_TAG)
        set(UNIFIED_RUNTIME_TAG "${BENCHMARK_UR_OVERRIDE_FETCH_CONTENT_TAG}")
    endif()

    message(STATUS
      "Will fetch Unified Runtime from ${UNIFIED_RUNTIME_REPO} at ${UNIFIED_RUNTIME_TAG}")

    FetchContent_Declare(unified-runtime
    GIT_REPOSITORY    ${UNIFIED_RUNTIME_REPO}
    GIT_TAG           ${UNIFIED_RUNTIME_TAG}
    )

    FetchContent_GetProperties(unified-runtime)
    FetchContent_MakeAvailable(unified-runtime)

endif()

# Define target
set(API_NAME ur)
set(TARGET_NAME compute_benchmarks_framework_${API_NAME})
add_library(${TARGET_NAME} STATIC ur.cpp ur.h error.h)
target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${TARGET_NAME} PUBLIC compute_benchmarks_framework ur_loader ur_headers)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER framework)
setup_vs_folders(${TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR})
setup_warning_options(${TARGET_NAME})
setup_output_directory(${TARGET_NAME})
if (MSVC)
    set_target_properties(${TARGET_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# Add this API to global array
set_property(GLOBAL APPEND PROPERTY APIS ${API_NAME})
