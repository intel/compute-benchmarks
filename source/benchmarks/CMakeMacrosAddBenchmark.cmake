#
# Copyright (C) 2022-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

function (add_benchmark BASE_TARGET_NAME)
    get_property(ALL_APIS GLOBAL PROPERTY APIS)

    # Rest of the arguments contain enabled APIs. We first filter them according to global CMake settings
    set(ENABLED_APIS)
    set(HAS_ALL_API_BINARY OFF)
    foreach(API ${ARGN})
        if (${API} STREQUAL "all" AND BUILD_ALL_API_BINARIES)
            set(HAS_ALL_API_BINARY ON)
            list(APPEND ENABLED_APIS ${API})
        endif()
        if (BUILD_SINGLE_API_BINARIES)
            list(FIND ALL_APIS ${API} FOUND)
            if (NOT "${FOUND}" EQUAL "-1")
                list(APPEND ENABLED_APIS ${API})
            endif()
        endif()
    endforeach()

    # Add benchmarks for each enabled API
    foreach(API ${ENABLED_APIS})
        if (${API} STREQUAL "all")
            set(APPEND_API_TO_TARGET_NAME OFF)
            set(APIS "${ALL_APIS}")
            set(REGISTER_FOR_DOCS_GENERATION OFF)
        else()
            set(APPEND_API_TO_TARGET_NAME ON)
            set(APIS "${API}")
            set(REGISTER_FOR_DOCS_GENERATION ON)
        endif()
        add_benchmark_for_api(${BASE_TARGET_NAME} ${APPEND_API_TO_TARGET_NAME} ${REGISTER_FOR_DOCS_GENERATION} "${APIS}")
    endforeach()
endfunction()

function (add_benchmark_for_api BASE_TARGET_NAME APPEND_API_TO_TARGET_NAME REGISTER_FOR_DOCS_GENERATION APIS)
    # Get names
    set(TARGET_NAME "${BASE_TARGET_NAME}")
    set(TARGET_FOLDER_NAME "tests")
    if (APPEND_API_TO_TARGET_NAME)
        string(APPEND TARGET_NAME "_${APIS}")
    endif()
    set(BENCHMARK_LIST "${BENCHMARK_LIST};${TARGET_NAME}" CACHE STRING "Benchmark target list" FORCE)

    # Log benchmark name
    if (LOG_BENCHMARK_TARGETS)
        message(STATUS "Adding benchmark: ${TARGET_NAME}")
    endif()

    # Register for docs generations
    if (REGISTER_FOR_DOCS_GENERATION)
        set_property(GLOBAL APPEND PROPERTY TARGETS_FOR_DOCS_GENERATION ${TARGET_NAME})
    endif()

    # Define target
    add_executable(${TARGET_NAME} CMakeLists.txt)
    foreach(API ${APIS})
        target_link_libraries(${TARGET_NAME} PRIVATE compute_benchmarks_framework_${API})
    endforeach()
    target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    set_target_properties(${TARGET_NAME} PROPERTIES FOLDER ${TARGET_FOLDER_NAME})
    set_property(GLOBAL PROPERTY ${TARGET_NAME}_LOCATION "")
    if (BUILD_FOR_PUBLISHING)
        set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${BASE_TARGET_NAME})
    endif()
    if (BUILD_FOR_PUBLISHING OR INCLUDE_VERSION)
        target_compile_definitions(${TARGET_NAME} PUBLIC BENCHMARK_VERSION="${BENCHMARK_VERSION}")
    else()
        target_compile_definitions(${TARGET_NAME} PUBLIC BENCHMARK_VERSION="")
    endif()
    if (HAVE_SYS_PIDFD_GETFD AND HAVE_SYS_PIDFD_OPEN)
        target_compile_definitions(${TARGET_NAME} PUBLIC USE_PIDFD)
    endif()

    # API agnostic sources
    set(API_AGNOSTIC_SOURCE_DIRECTORIES
        ${BENCHMARKS_SOURCE_ROOT}/common
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/gtest
        ${CMAKE_CURRENT_SOURCE_DIR}/definitions
        ${CMAKE_CURRENT_SOURCE_DIR}/utility
    )
    foreach(DIR ${API_AGNOSTIC_SOURCE_DIRECTORIES})
        add_sources_to_benchmark(${TARGET_NAME} ${DIR})
    endforeach()
    add_kernels_to_benchmark(${TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/kernels)

    # API specific sources
    set(API_SPECIFIC_SOURCE_DIRECTORIES
        ${BENCHMARKS_SOURCE_ROOT}/common
        ${CMAKE_CURRENT_SOURCE_DIR}/implementations
        ${CMAKE_CURRENT_SOURCE_DIR}/utility
    )
    foreach(API ${APIS})
        foreach(PARENT_DIR ${API_SPECIFIC_SOURCE_DIRECTORIES})
            if (${API} STREQUAL "syclpreview")
                set(DIR ${PARENT_DIR}/sycl)
            else()
                set(DIR ${PARENT_DIR}/${API})
            endif()
            message(STATUS "Adding sources from ${DIR} to ${TARGET_NAME}")
            add_sources_to_benchmark(${TARGET_NAME} ${DIR})
        endforeach()
    endforeach()

    # Create kernel copy targets
    set(COPY_KERNEL_TARGET_NAME "copy_kernel_files_${BASE_TARGET_NAME}")
    if(NOT TARGET ${COPY_KERNEL_TARGET_NAME})
        add_custom_target(${COPY_KERNEL_TARGET_NAME})
        set_target_properties(${COPY_KERNEL_TARGET_NAME} PROPERTIES FOLDER "copy_kernel_files")
        set(KERNEL_OUTPUT_DIR ${OUTPUT_DIR})
        copy_kernels_to_bin_directory_of_target(${COPY_KERNEL_TARGET_NAME} ${TARGET_NAME} ${KERNEL_OUTPUT_DIR})
    endif()
    add_dependencies(${TARGET_NAME} ${COPY_KERNEL_TARGET_NAME})

    # Additional setup
    setup_vs_folders(${TARGET_NAME} ${BENCHMARKS_SOURCE_ROOT})
    setup_warning_options(${TARGET_NAME})
    setup_output_directory(${TARGET_NAME})
endfunction()

function(add_benchmark_dependency_on_workload BENCHMARK_BASE_NAME WORKLOAD API)
    foreach(BENCHMARK "${BENCHMARK_BASE_NAME}_${API}" "${BENCHMARK_BASE_NAME}")
        if (NOT TARGET ${WORKLOAD})
            message(FATAL_ERROR "Workload \"${WORKLOAD}\" does not exist")
        endif()
        if (TARGET "${BENCHMARK}")
            add_dependencies(${BENCHMARK} ${WORKLOAD})
            add_custom_command(
                TARGET ${BENCHMARK}
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                $<TARGET_FILE:${WORKLOAD}>
                $<TARGET_FILE_DIR:${BENCHMARK}>/$<TARGET_FILE_NAME:${WORKLOAD}>
            )
            get_target_property(KERNEL_OUTPUT_DIR ${BENCHMARK} RUNTIME_OUTPUT_DIRECTORY)
            copy_kernels_to_bin_directory_of_target(${BENCHMARK} ${WORKLOAD} ${KERNEL_OUTPUT_DIR})
        endif()
    endforeach()
endfunction()
