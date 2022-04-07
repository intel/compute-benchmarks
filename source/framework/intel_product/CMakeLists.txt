#
# Copyright (C) 2022 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

file(GLOB_RECURSE SOURCES *.cpp *.h *inl)
target_sources(${TARGET_NAME} PRIVATE ${SOURCES})
