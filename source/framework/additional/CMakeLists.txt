#
# Copyright (C) 2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

set(ADDITIONAL_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}${BRANCH_DIR_SUFFIX}api_additional.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}${BRANCH_DIR_SUFFIX}additional_configuration.h
    ${CMAKE_CURRENT_SOURCE_DIR}${BRANCH_DIR_SUFFIX}additional_configuration.cpp
)

target_sources(${TARGET_NAME} PRIVATE ${ADDITIONAL_FILES})
target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}${BRANCH_DIR_SUFFIX})
