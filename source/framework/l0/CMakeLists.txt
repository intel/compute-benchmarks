#
# Copyright (C) 2022-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

if (NOT BUILD_L0)
    return()
endif()

# Get libraries and headers for the SDK
if(USE_SYSTEM_LEVEL_ZERO)
    find_package(LevelZero QUIET)
endif()
if(NOT LevelZero_FOUND)
    if (WIN32)
        set(LIB_NAME ze_loader.lib)
    else()
        set(LIB_NAME libze_loader.so)
    endif()
    set(LevelZero_LIBRARIES "${CMAKE_SOURCE_DIR}/third_party/level-zero-sdk/lib/x64/${LIB_NAME}")
    set(LevelZero_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/third_party/level-zero-sdk/include" "${CMAKE_SOURCE_DIR}/third_party/level-zero-sdk/include/level_zero")
    message(STATUS "Using Level Zero from SDK sources")
else()
    message(STATUS "Level Zero installation found in ${LevelZero_LIBRARIES_DIR}")
endif()

# Get sources
file(GLOB_RECURSE SOURCES *.cpp *.h)

# Define target
set(API_NAME l0)
set(TARGET_NAME compute_benchmarks_framework_${API_NAME})
add_library(${TARGET_NAME} STATIC ${SOURCES})
if (NULL_L0)
    target_compile_definitions(${TARGET_NAME} PRIVATE NULL_L0)
else ()
    target_link_libraries(${TARGET_NAME} PUBLIC ${LevelZero_LIBRARIES})
endif ()
target_link_libraries(${TARGET_NAME} PUBLIC compute_benchmarks_framework)
target_include_directories(${TARGET_NAME} PUBLIC ${LevelZero_INCLUDE_DIRS})
target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/third_party/level-zero-intel)
target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/third_party/level-zero-sdk/include)
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
