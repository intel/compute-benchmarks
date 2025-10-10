#
# Copyright (C) 2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

set(KERNELS_SOURCE_DIR "${CMAKE_SOURCE_DIR}/source/kernels")
set(KERNELS_OUTPUT_DIR "${OUTPUT_DIR}")

file(GLOB_RECURSE KERNEL_CL_FILES CONFIGURE_DEPENDS
    "${KERNELS_SOURCE_DIR}/*.cl"
)

file(GLOB_RECURSE KERNEL_SPV_FILES CONFIGURE_DEPENDS
    "${KERNELS_SOURCE_DIR}/*.spv"
)

function(copy_kernel_files)
    file(MAKE_DIRECTORY "${KERNELS_OUTPUT_DIR}")
    foreach(KERNEL_FILE ${ARGN})
        configure_file("${KERNEL_FILE}" "${KERNELS_OUTPUT_DIR}" COPYONLY)
    endforeach()
endfunction()

copy_kernel_files(${KERNEL_CL_FILES} ${KERNEL_SPV_FILES})