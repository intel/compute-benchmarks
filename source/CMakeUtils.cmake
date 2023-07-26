#
# Copyright (C) 2022-2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

function(copy_kernels_to_bin_directory_of_target TARGET_NAME TARGET_KERNEL_OWNER TARGET_DIR)
    get_target_property(KERNELS ${TARGET_KERNEL_OWNER} KERNELS)
    if (KERNELS STREQUAL "KERNELS-NOTFOUND")
        return()
    endif()
    foreach(KERNEL ${KERNELS})
        get_filename_component(KERNEL_NAME ${KERNEL} NAME)
        add_custom_command(
            TARGET ${TARGET_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${KERNEL}
            ${TARGET_DIR}/${KERNEL_NAME}
        )
    endforeach()
endfunction()

function(setup_vs_folders TARGET_NAME BASE_DIR)
    if (MSVC)
        get_target_property(SOURCES ${TARGET_NAME} SOURCES)
        source_group (TREE ${BASE_DIR} FILES ${SOURCES})
    endif()
endfunction()

function(setup_warning_options TARGET_NAME)
    target_compile_options(${TARGET_NAME} PRIVATE
      $<$<CXX_COMPILER_ID:MSVC>:/W4>
      $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Wno-missing-field-initializers -Wno-unknown-cuda-version>
    )

    if(NOT ALLOW_WARNINGS)
        target_compile_options(${TARGET_NAME} PRIVATE
          $<$<CXX_COMPILER_ID:MSVC>:/WX>
          $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Werror>
        )
    endif()
endfunction(setup_warning_options)

function(setup_output_directory TARGET_NAME)
    set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
    set_target_properties(${TARGET_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
    set_target_properties(${TARGET_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
endfunction()

function(negate_flag INPUT OUTPUT)
    if(INPUT)
        set(${OUTPUT} OFF PARENT_SCOPE)
    else()
        set(${OUTPUT} ON PARENT_SCOPE)
    endif()
endfunction()

function(add_subdirectories)
  file(GLOB SUB_DIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)
  foreach(SUB_DIR ${SUB_DIRS})
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${SUB_DIR}/CMakeLists.txt)
      continue()
    endif()
    if(NOT WIN32 AND SUB_DIR STREQUAL windows)
        continue()
    endif()
    if(NOT UNIX AND SUB_DIR STREQUAL linux)
        continue()
    endif()
    add_subdirectory(${SUB_DIR})
  endforeach()
endfunction()

function(add_sources_to_benchmark TARGET_NAME DIR)
    file(GLOB SOURCES ${DIR}/*.cpp ${DIR}/*.h)
    target_sources(${TARGET_NAME} PRIVATE ${SOURCES})
endfunction()

function (add_kernels_to_benchmark TARGET_NAME DIR)
    file(GLOB_RECURSE SOURCE_KERNELS ${DIR}/*.cl)
    target_sources(${TARGET_NAME} PRIVATE ${SOURCE_KERNELS})

    file(GLOB KERNELS ${DIR}/*.spv ${DIR}/copyable_sources/*.cl ${DIR}/copyable_sources/*.spv)
    set_property(TARGET ${TARGET_NAME} APPEND PROPERTY KERNELS ${KERNELS})
endfunction()
