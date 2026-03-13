#
# Copyright (C) 2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

include(FindPackageHandleStandardArgs)

SET(UR_SEARCH_PATHS
  /usr
)

find_path(UR_INCLUDE_DIR
  NAMES ur_api.h
  PATHS ${UR_SEARCH_PATHS}
  PATH_SUFFIXES "include" "include/unified-runtime"
)

find_library(UR_LIBRARY
  NAMES ur_loader
  PATHS ${UR_SEARCH_PATHS}
  PATH_SUFFIXES "lib" "lib64"
)

find_package_handle_standard_args(unified-runtime
  REQUIRED_VARS
    UR_INCLUDE_DIR
    UR_LIBRARY
  REASON_FAILURE_MESSAGE
    "If you want to use your UR installation, set CMAKE_PREFIX_PATH accordingly"
)

if(unified-runtime_FOUND)
  # make sure to get the proper dir, no matter where we find the header file
  if(UR_INCLUDE_DIR MATCHES "unified-runtime$" OR UR_INCLUDE_DIR MATCHES "unified-runtime/$")
    cmake_path(GET UR_INCLUDE_DIR PARENT_PATH UR_INCLUDE_DIR)
  endif()

  set(UR_INCLUDE_DIRS ${UR_INCLUDE_DIR})
  set(UR_LIBRARIES ${UR_LIBRARY})
endif()
