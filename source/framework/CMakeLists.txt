#
# Copyright (C) 2022-2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

# Define Framework library
file(GLOB SOURCES *.cpp *.h)
set(TARGET_NAME compute_benchmarks_framework)
add_library(${TARGET_NAME} STATIC ${SOURCES})
target_link_libraries(${TARGET_NAME} PUBLIC gtest)
if (UNIX)
    target_link_libraries(${TARGET_NAME} PUBLIC stdc++fs)
endif()
target_include_directories(${TARGET_NAME} PUBLIC ${SOURCE_ROOT})
target_compile_features(${TARGET_NAME} PUBLIC cxx_std_17)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER framework)
add_subdirectories()

# Ignore CRT Warnings
if (MSVC)
    target_compile_definitions(${TARGET_NAME} PUBLIC _CRT_SECURE_NO_WARNINGS)
endif()

# Export all symbols on VS builds (so __dllexport is not required)
if (MSVC)
    set_target_properties(${TARGET_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

if (HAVE_SYS_PIDFD_GETFD AND HAVE_SYS_PIDFD_OPEN)
    target_compile_definitions(${TARGET_NAME} PUBLIC USE_PIDFD)
endif()

# Additional setup
setup_vs_folders(${TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR})
setup_warning_options(${TARGET_NAME})
setup_output_directory(${TARGET_NAME})
