/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#if __has_include(<filesystem>)
#include <filesystem>
namespace FileSystem = std::filesystem;
#else
#include <experimental/filesystem>
namespace FileSystem = std::experimental::filesystem;
#endif

#include <filesystem>
#include <string>

struct WorkingDirectoryHelper {
    static FileSystem::path getExeLocation();

    static void changeDirectory(const FileSystem::path &directory) {
        FileSystem::current_path(directory);
    }

    static void changeDirectoryToExeDirectory() {
        const FileSystem::path exeLocation = getExeLocation();
        const FileSystem::path exeDirectory = exeLocation.parent_path();
        changeDirectory(exeDirectory);
    }
};
