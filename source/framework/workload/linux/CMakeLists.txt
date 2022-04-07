#
# Copyright (C) 2022 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

file(GLOB SOURCES *.cpp *.h)
target_sources(${TARGET_NAME} PRIVATE ${SOURCES})
