#
# Copyright (C) 2022 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

if (NOT BUILD_TOOLS)
    return()
endif()

add_subdirectory(mutex_comparison)
add_subdirectory(show_devices_ocl)
add_subdirectory(show_devices_l0)
add_subdirectory(show_devices_sycl)
