#
# Copyright (C) 2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

if (NOT BUILD_VK)
    return()
endif()

# Get libraries and headers for the SDK
if(USE_SYSTEM_VULKAN)
    find_package(Vulkan QUIET)
endif()
if(NOT Vulkan_FOUND)
    set(ADDITIONAL_PATH "${CMAKE_SOURCE_DIR}/third_party/vulkan-sdk")
    message(STATUS "Vulkan installation not found. Using ${ADDITIONAL_PATH}")

    if (WIN32)
        set(LIB_NAME vulkan-1.lib)
    else()
        set(LIB_NAME libvulkan.so)
    endif()
    set(Vulkan_LIBRARIES    "${ADDITIONAL_PATH}/lib/x64/${LIB_NAME}")
    set(Vulkan_INCLUDE_DIRS "${ADDITIONAL_PATH}/include")
else()
    message(STATUS "Vulkan installation found")
endif()

# Get sources
file(GLOB_RECURSE SOURCES *.cpp *.h)

# Define target
set(API_NAME vk)
set(TARGET_NAME compute_benchmarks_framework_${API_NAME})
add_library(${TARGET_NAME} STATIC ${SOURCES})
target_link_libraries(${TARGET_NAME} PUBLIC ${Vulkan_LIBRARIES})
target_link_libraries(${TARGET_NAME} PUBLIC compute_benchmarks_framework)
target_include_directories(${TARGET_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
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
