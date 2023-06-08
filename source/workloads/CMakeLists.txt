#
# Copyright (C) 2022-2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

function(add_workload TARGET_NAME API)
    add_executable(${TARGET_NAME} CMakeLists.txt)
    target_link_libraries(${TARGET_NAME} PRIVATE compute_benchmarks_framework_${API})
    target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    set_target_properties(${TARGET_NAME} PROPERTIES FOLDER workloads)
    if (BUILD_FOR_PUBLISHING)
        set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/workloads/${BASE_TARGET_NAME})
    endif()
    if (HAVE_SYS_PIDFD_GETFD AND HAVE_SYS_PIDFD_OPEN)
        target_compile_definitions(${TARGET_NAME} PUBLIC USE_PIDFD)
    endif()

    # Add sources
    add_sources_to_benchmark(${TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR})
    add_kernels_to_benchmark(${TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/kernels)
    get_target_property(KERNEL_OUTPUT_DIR ${TARGET_NAME} RUNTIME_OUTPUT_DIRECTORY)
    copy_kernels_to_bin_directory_of_target(${TARGET_NAME} ${TARGET_NAME} ${KERNEL_OUTPUT_DIR})

    # Additional config
    setup_vs_folders(${TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR})
    setup_warning_options(${TARGET_NAME})
    setup_output_directory(${TARGET_NAME})
endfunction()

add_subdirectory(init_workload_l0)
add_subdirectory(heat3d_workload_l0)
add_subdirectory(single_queue_workload_l0)
add_subdirectory(single_queue_workload_shared_buffer_l0)
add_subdirectory(immediate_cmdlist_copy_workload_l0)
add_subdirectory(immediate_cmdlist_walker_submission_workload_l0)
if (BUILD_HELLO_WORLD)
    add_subdirectory(hello_world_template_workload_ocl)
    add_subdirectory(hello_world_workload_ocl)
endif()
