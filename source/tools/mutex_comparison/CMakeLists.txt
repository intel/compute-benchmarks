#
# Copyright (C) 2022 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

set(TARGET_NAME mutex_comparison)
add_executable(${TARGET_NAME} CMakeLists.txt)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER tools)
add_sources_to_benchmark(${TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(${TARGET_NAME} PRIVATE cxx_std_17)
if(UNIX)
target_link_libraries(${TARGET_NAME} pthread)
endif()

# Additional config
setup_vs_folders(${TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR})
setup_output_directory(${TARGET_NAME})
