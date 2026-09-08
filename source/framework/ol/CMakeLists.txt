#
# Copyright (C) 2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

if (NOT BUILD_OL)
    return()
endif()

set(LIBOFFLOAD_LIBRARY_DIR "" CACHE PATH "LLVM installation library directory containing libLLVMOffload")
set(LIBOFFLOAD_INCLUDE_DIR "" CACHE PATH "LLVM installation include directory containing offload/OffloadAPI.h")

if (NOT LIBOFFLOAD_LIBRARY_DIR)
    message(FATAL_ERROR "BUILD_OL requires LIBOFFLOAD_LIBRARY_DIR")
endif()
if (NOT LIBOFFLOAD_INCLUDE_DIR)
    message(FATAL_ERROR "BUILD_OL requires LIBOFFLOAD_INCLUDE_DIR")
endif()

unset(LIBOFFLOAD_LIBRARY CACHE)
find_library(LIBOFFLOAD_LIBRARY
    NAMES LLVMOffload
    PATHS "${LIBOFFLOAD_LIBRARY_DIR}"
    NO_DEFAULT_PATH
)
if (NOT LIBOFFLOAD_LIBRARY)
    message(FATAL_ERROR "liboffload was not found in LIBOFFLOAD_LIBRARY_DIR=${LIBOFFLOAD_LIBRARY_DIR}")
endif()

unset(LIBOFFLOAD_API_INCLUDE_DIR CACHE)
find_path(LIBOFFLOAD_API_INCLUDE_DIR
    NAMES OffloadAPI.h
    PATHS "${LIBOFFLOAD_INCLUDE_DIR}"
    PATH_SUFFIXES offload
    NO_DEFAULT_PATH
)
if (NOT LIBOFFLOAD_API_INCLUDE_DIR)
    message(FATAL_ERROR "OffloadAPI.h was not found in LIBOFFLOAD_INCLUDE_DIR=${LIBOFFLOAD_INCLUDE_DIR}")
endif()

set(API_NAME ol)
set(TARGET_NAME compute_benchmarks_framework_${API_NAME})
add_library(${TARGET_NAME} STATIC
    error.h
    offload_print.h
    ol.cpp
    ol.h
    print_device_info.cpp
    print_device_info.h
)

target_include_directories(${TARGET_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LIBOFFLOAD_API_INCLUDE_DIR}
)
target_link_libraries(${TARGET_NAME} PUBLIC
    compute_benchmarks_framework
    ${LIBOFFLOAD_LIBRARY}
)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER framework)
setup_vs_folders(${TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR})
setup_warning_options(${TARGET_NAME})
setup_output_directory(${TARGET_NAME})
if (MSVC)
    set_target_properties(${TARGET_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

set_property(GLOBAL APPEND PROPERTY APIS ${API_NAME})
