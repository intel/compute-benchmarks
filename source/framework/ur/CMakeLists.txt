#
# Copyright (C) 2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

if (NOT BUILD_UR)
    return()
endif()

include(FetchContent)

set(UMF_BUILD_EXAMPLES OFF CACHE INTERNAL "EXAMPLES")
set(UR_BUILD_TESTS OFF CACHE INTERNAL "TESTS")

set(UNIFIED_RUNTIME_REPO "https://github.com/oneapi-src/unified-runtime.git")
# commit 3d8fe8d298cec8db624fc230fa5c0e19865aa6f1
# Merge: e8c9f3db 0a259f33
# Author: Piotr Balcer <piotr.balcer@intel.com>
# Date:   Thu Jul 25 15:13:00 2024 +0200
#     Merge pull request #1430 from igchor/umf_dynamic_linking
#     Bump UMF version and switch to dynamic linking
set(UNIFIED_RUNTIME_TAG 3d8fe8d298cec8db624fc230fa5c0e19865aa6f1)

if(BENCHMARK_UR_OVERRIDE_FETCH_CONTENT_REPO)
    set(UNIFIED_RUNTIME_REPO "${BENCHMARK_UR_OVERRIDE_FETCH_CONTENT_REPO}")
endif()
if(BENCHMARK_UR_OVERRIDE_FETCH_CONTENT_TAG)
    set(UNIFIED_RUNTIME_TAG "${BENCHMARK_UR_OVERRIDE_FETCH_CONTENT_TAG}")
endif()

# Define target
set(API_NAME ur)

message(STATUS "Will fetch Unified Runtime from ${UNIFIED_RUNTIME_REPO}")
FetchContent_Declare(unified-runtime
GIT_REPOSITORY    ${UNIFIED_RUNTIME_REPO}
GIT_TAG           ${UNIFIED_RUNTIME_TAG}
)

FetchContent_GetProperties(unified-runtime)
FetchContent_MakeAvailable(unified-runtime)

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
