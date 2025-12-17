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
    derive_benchmark_source_path("${CMAKE_CURRENT_SOURCE_DIR}" BENCHMARK_SOURCE_PATH)

    add_executable(${TARGET_NAME} CMakeLists.txt)
    foreach(API ${APIS})
        target_link_libraries(${TARGET_NAME} PRIVATE compute_benchmarks_framework_${API})
    endforeach()
    target_include_directories(${TARGET_NAME} PRIVATE ${BENCHMARK_SOURCE_PATH})
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
        ${BENCHMARK_SOURCE_PATH}
        ${BENCHMARK_SOURCE_PATH}/gtest
        ${BENCHMARK_SOURCE_PATH}/definitions
        ${BENCHMARK_SOURCE_PATH}/utility
    )
    foreach(DIR ${API_AGNOSTIC_SOURCE_DIRECTORIES})
        add_sources_to_benchmark(${TARGET_NAME} ${DIR})
    endforeach()

    # API specific sources
    foreach(API ${APIS})
        set(API_DIR ${API})
        if (${API} STREQUAL "syclpreview")
            set(API_DIR "sycl")
        endif()

        foreach(BRANCH_DIR ${BRANCH_DIR_LIST})
            set(API_SUBPATH "${BRANCH_DIR}${API_DIR}")

            set(COMMON_DIR "${BENCHMARKS_SOURCE_ROOT}/common/${API_SUBPATH}")
            set(IMPL_DIR "${BENCHMARK_SOURCE_PATH}/implementations/${API_SUBPATH}")
            set(UTIL_DIR "${BENCHMARK_SOURCE_PATH}/utility/${API_SUBPATH}")

            get_filename_component(COMMON_DIR "${COMMON_DIR}" ABSOLUTE)
            get_filename_component(IMPL_DIR "${IMPL_DIR}" ABSOLUTE)
            get_filename_component(UTIL_DIR "${UTIL_DIR}" ABSOLUTE)
            
            foreach(DIR ${COMMON_DIR} ${IMPL_DIR} ${UTIL_DIR})
                if(EXISTS "${DIR}")
                    message(STATUS "Adding sources from ${DIR} to ${TARGET_NAME}")
                    add_sources_to_benchmark(${TARGET_NAME} ${DIR})
                endif()
            endforeach()
        endforeach()
    endforeach()

    # Additional setup
    setup_vs_folders(${TARGET_NAME} ${BENCHMARKS_SOURCE_ROOT})
    setup_warning_options(${TARGET_NAME})
    setup_output_directory(${TARGET_NAME})
endfunction()

function(add_benchmark_dependency_on_workload BENCHMARK_BASE_NAME WORKLOAD API)
    get_property(ALL_APIS GLOBAL PROPERTY APIS)
    list(FIND ALL_APIS ${API} API_ENABLED)
    if (API_ENABLED STREQUAL "-1")
        message(STATUS "${BENCHMARK_BASE_NAME}_${API} won't be added as ${API} is disabled")
        return()
    endif()
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
        endif()
    endforeach()
endfunction()

function(derive_benchmark_source_path CURRENT_DIR OUT_BENCHMARK_SOURCE_PATH)
    get_filename_component(BENCHMARK_ROOT "${CURRENT_DIR}" DIRECTORY)
    get_filename_component(CURRENT_DIR_NAME "${CURRENT_DIR}" NAME)

    set(RESOLVED_BASE_TARGET "${BASE_TARGET_NAME}")

    if (DEFINED BRANCH_TYPE AND NOT "${BRANCH_TYPE}" STREQUAL "")
        string(REGEX REPLACE "_${BRANCH_TYPE}$" "" RESOLVED_BASE_TARGET "${RESOLVED_BASE_TARGET}")

        if ("${CURRENT_DIR_NAME}" STREQUAL "${BRANCH_TYPE}")
            get_filename_component(BENCHMARK_ROOT "${BENCHMARK_ROOT}" DIRECTORY)
        endif()
    endif()

    set(${OUT_BENCHMARK_SOURCE_PATH} "${BENCHMARK_ROOT}/${RESOLVED_BASE_TARGET}" PARENT_SCOPE)
endfunction()
