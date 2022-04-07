#
# Copyright (C) 2022 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

if (NOT BUILD_OCL)
    return()
endif()

# Get libraries and headers for the SDK
find_package(OpenCL QUIET)
if(NOT ${OpenCL_FOUND})
    set(ADDITIONAL_PATH "${CMAKE_SOURCE_DIR}/third_party/opencl-sdk")
    message(STATUS "OpenCL installation not found. Using ${ADDITIONAL_PATH}")

    if (WIN32)
        set(LIB_NAME OpenCL.lib)
    else()
        set(LIB_NAME libOpenCL.so)
    endif()
    set(OpenCL_LIBRARIES    "${ADDITIONAL_PATH}/lib/x64/${LIB_NAME}")
    set(OpenCL_INCLUDE_DIRS "${ADDITIONAL_PATH}/include")
else()
    message(STATUS "OpenCL installation found")
endif()

# Get sources
file(GLOB_RECURSE SOURCES *.cpp *.h)

# Define target
set(API_NAME ocl)
set(TARGET_NAME compute_benchmarks_framework_${API_NAME})
add_library(${TARGET_NAME} STATIC ${SOURCES})
target_link_libraries(${TARGET_NAME} PUBLIC ${OpenCL_LIBRARIES})
target_link_libraries(${TARGET_NAME} PUBLIC compute_benchmarks_framework)
target_include_directories(${TARGET_NAME} PUBLIC ${OpenCL_INCLUDE_DIRS})
target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/third_party/opencl-intel)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER framework)
add_subdirectories()
setup_vs_folders(${TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR})
setup_warning_options(${TARGET_NAME})
setup_output_directory(${TARGET_NAME})
if (MSVC)
    set_target_properties(${TARGET_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# Add this API to global array
set_property(GLOBAL APPEND PROPERTY APIS ${API_NAME})
