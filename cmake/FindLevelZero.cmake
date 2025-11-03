#
# Copyright (C) 2022-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

include(FindPackageHandleStandardArgs)

SET(LevelZero_SEARCH_PATHS
  /usr
)

find_path(LevelZero_INCLUDE_DIR
  NAMES level_zero/ze_api.h
  PATHS ${LevelZero_SEARCH_PATHS}
  PATH_SUFFIXES "include"
)

find_library(LevelZero_LIBRARY
  NAMES ze_loader
  PATHS ${LevelZero_SEARCH_PATHS}
  PATH_SUFFIXES "lib" "lib/"
)

find_package_handle_standard_args(LevelZero
  REQUIRED_VARS
    LevelZero_INCLUDE_DIR
    LevelZero_LIBRARY
)

if(LevelZero_FOUND)
  cmake_path(GET LevelZero_LIBRARY PARENT_PATH LevelZero_LIBRARIES_PATH)
  list(APPEND LevelZero_LIBRARIES ${LevelZero_LIBRARY} ${CMAKE_DL_LIBS})
  list(APPEND LevelZero_INCLUDE_DIRS ${LevelZero_INCLUDE_DIR})
  set(LevelZero_LIBRARIES_DIR ${LevelZero_LIBRARIES_PATH})
endif()
