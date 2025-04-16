#
# Copyright (C) 2022-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

if (NOT BUILD_SYCL)
    return()
endif()

# Get libraries and headers for the SDK
if(NOT SYCL_FOUND)
    message(WARNING "SYCL not found - SYCL benchmarks will not be built")
else()
    # Get sources
    file(GLOB_RECURSE SOURCES *.cpp *.h)

    # Function to create SYCL library
    function(create_sycl_library api_name additional_flags)
        # Define target
        separate_arguments(SYCL_CFLAGS)
        separate_arguments(SYCL_LFLAGS)
        set(TARGET_NAME compute_benchmarks_framework_${api_name})
        add_library(${TARGET_NAME} STATIC ${SOURCES})
        target_link_libraries(${TARGET_NAME} PUBLIC ${SYCL_LFLAGS} ${additional_flags})
        target_link_libraries(${TARGET_NAME} PUBLIC compute_benchmarks_framework)
        target_include_directories(${TARGET_NAME} PUBLIC ${SYCL_INCLUDE_DIR} ${SYCL_INCLUDE_DIR}/sycl)
        target_compile_options(${TARGET_NAME} PUBLIC ${SYCL_CFLAGS} ${additional_flags})
        set_target_properties(${TARGET_NAME} PROPERTIES FOLDER framework)
        add_subdirectories()
        setup_vs_folders(${TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR})
        setup_warning_options(${TARGET_NAME})
        setup_output_directory(${TARGET_NAME})
        if (MSVC)
            set_target_properties(${TARGET_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
        endif()

        # Add this API to global array
        set_property(GLOBAL APPEND PROPERTY APIS ${api_name})
    endfunction()

    # Create libraries for sycl and sycl_preview
    create_sycl_library(sycl "")
    create_sycl_library(syclpreview "-fpreview-breaking-changes")

endif()
