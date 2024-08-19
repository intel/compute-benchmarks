#
# Copyright (C) 2022 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

file(GLOB SOURCES *.cpp *.h)
target_sources(${TARGET_NAME} PRIVATE ${SOURCES})
target_include_directories(${TARGET_NAME} PUBLIC ${SOURCE_ROOT})
target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/third_party/googletest/googletest/include)
add_subdirectories()
