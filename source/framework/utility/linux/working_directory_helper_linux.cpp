/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/utility/error.h"
#include "framework/utility/linux/error.h"
#include "framework/utility/working_directory_helper.h"

#include <unistd.h>

FileSystem::path WorkingDirectoryHelper::getExeLocation() {
    char buffer[4096];
    auto retval = readlink("/proc/self/exe", buffer, sizeof(buffer));
    FATAL_ERROR_IF_SYS_CALL_FAILED(retval, "retrieving .exe location");
    if (retval < static_cast<int64_t>(sizeof(buffer))) {
        buffer[retval] = 0;
    } else {
        FATAL_ERROR_IF_SYS_CALL_FAILED(-1, "path truncation while retrieving .exe location");
    }
    return FileSystem::path{buffer};
}
