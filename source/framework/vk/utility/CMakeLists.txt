#
# Copyright (C) 2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

file(GLOB SOURCES *.cpp *.h)
target_sources(${TARGET_NAME} PRIVATE ${SOURCES})
add_subdirectories()
