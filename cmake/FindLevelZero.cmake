#
# Copyright (C) 2022 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

include(FindPackageHandleStandardArgs)

find_path(LevelZero_INCLUDE_DIR
  NAMES level_zero/ze_api.h
  PATHS
    ${L0_ROOT}
  PATH_SUFFIXES "include"
)

find_library(LevelZero_LIBRARY
  NAMES ze_loader ze_loader32 ze_loader64
  PATHS
    ${L0_ROOT}
  PATH_SUFFIXES "lib" "lib/"
)

find_package_handle_standard_args(LevelZero
  REQUIRED_VARS
    LevelZero_INCLUDE_DIR
    LevelZero_LIBRARY
  HANDLE_COMPONENTS
)
mark_as_advanced(LevelZero_LIBRARY LevelZero_INCLUDE_DIR)

if(LevelZero_FOUND)
    list(APPEND LevelZero_LIBRARIES ${LevelZero_LIBRARY} ${CMAKE_DL_LIBS})
    list(APPEND LevelZero_INCLUDE_DIRS ${LevelZero_INCLUDE_DIR})
endif()
