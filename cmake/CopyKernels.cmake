#
# Copyright (C) 2025-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

set(KERNELS_SOURCE_DIR "${CMAKE_SOURCE_DIR}/source/kernels")
set(KERNELS_OUTPUT_DIR "${OUTPUT_DIR}")

include(${CMAKE_MODULE_PATH}/${BRANCH_TYPE}/CopyKernelsAdditional.cmake OPTIONAL)

list(APPEND KERNEL_EXTENSIONS ".cl" ".spv")

set(KERNEL_FILES "")
foreach(EXTENSION ${KERNEL_EXTENSIONS})
    file(GLOB_RECURSE EXTENSION_FILES CONFIGURE_DEPENDS
        "${KERNELS_SOURCE_DIR}/*${EXTENSION}"
    )
    list(APPEND KERNEL_FILES ${EXTENSION_FILES})
endforeach()

file(MAKE_DIRECTORY "${KERNELS_OUTPUT_DIR}")
foreach(KERNEL_FILE ${KERNEL_FILES})
    configure_file("${KERNEL_FILE}" "${KERNELS_OUTPUT_DIR}" COPYONLY)
endforeach()
