/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/utility/error.h"
#include "framework/utility/windows/windows.h"
#include "framework/utility/working_directory_helper.h"

FileSystem::path WorkingDirectoryHelper::getExeLocation() {
    char buffer[4096];
    FATAL_ERROR_IF_SYS_CALL_FAILED(GetModuleFileNameA(nullptr, buffer, sizeof(buffer)), "retrieving .exe location");
    return FileSystem::path{buffer};
}
