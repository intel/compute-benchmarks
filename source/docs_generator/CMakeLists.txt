#
# Copyright (C) 2022 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

if (NOT GENERATE_DOCS)
    return()
endif()

# Prepare targets list
get_property(TARGETS_FOR_DOCS_GENERATION GLOBAL PROPERTY TARGETS_FOR_DOCS_GENERATION)
set(TARGETS "")
foreach(TGT ${TARGETS_FOR_DOCS_GENERATION})
    get_property(_location GLOBAL PROPERTY ${TGT}_LOCATION)
    string(APPEND TARGETS "{\"${_location}\", \"${TGT}\", \"$<TARGET_FILE:${TGT}>\"}, ")
endforeach()

# Setup executable target
set(TARGET_NAME docs_generator)
file(GLOB SOURCES *.cpp *.h)
add_executable(${TARGET_NAME} ${SOURCES})
target_link_libraries(${TARGET_NAME} PRIVATE compute_benchmarks_framework)
target_compile_definitions(${TARGET_NAME} PRIVATE TARGETS=${TARGETS})
target_compile_definitions(${TARGET_NAME} PRIVATE OUTPUT_DIR=${OUTPUT_DIR})
add_dependencies(${TARGET_NAME} ${TARGETS_FOR_DOCS_GENERATION})
setup_vs_folders(${TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR})

# Setup custom target to run generation on build
set(RUNNER_TARGET_NAME "run_${TARGET_NAME}")
add_custom_target(${RUNNER_TARGET_NAME}
    ALL
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_SOURCE_DIR}/TESTS.md
)
add_dependencies(${RUNNER_TARGET_NAME} ${TARGET_NAME})
add_custom_command(
    TARGET ${RUNNER_TARGET_NAME}
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:${TARGET_NAME}>"
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E env "LD_LIBRARY_PATH=$ENV{LD_LIBRARY_PATH}" "$<TARGET_FILE:${TARGET_NAME}>" --path=${CMAKE_SOURCE_DIR} --fileName=TESTS.md
)
