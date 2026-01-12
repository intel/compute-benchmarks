#
# Copyright (C) 2025-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

set(ADDITIONAL_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}${BRANCH_DIR_SUFFIX}helpers_additional.h
)

target_sources(${TARGET_NAME} PRIVATE ${ADDITIONAL_FILES})
target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}${BRANCH_DIR_SUFFIX})
