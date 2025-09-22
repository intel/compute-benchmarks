/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "file_helper.h"

#include <fstream>

std::vector<uint8_t> FileHelper::loadFile(const std::string &filePath, std::ios_base::openmode openMode) {
    std::ifstream stream(filePath, openMode);
    if (!stream.good()) {
        std::cerr << "ERROR: Failed to open file: " << filePath << ".\n";
        return {};
    }

    stream.seekg(0, stream.end);
    const size_t length = static_cast<size_t>(stream.tellg());
    stream.seekg(0, stream.beg);

    std::vector<uint8_t> content(length);
    stream.read(reinterpret_cast<char *>(content.data()), length);
    return content;
}

std::vector<uint8_t> FileHelper::loadBinaryFile(const std::string &filePath) {
    return loadFile(filePath, std::ios::in | std::ios::binary);
}

std::vector<uint8_t> FileHelper::loadTextFile(const std::string &filePath) {
    return loadFile(filePath, std::ios::in);
}

FileHelper::FileOrConsole::FileOrConsole(const std::string &filePath, std::ios::openmode openMode, std::ostream &fallback)
    : fallback(fallback) {
    if (!filePath.empty()) {
        this->ownedFile = std::make_unique<std::ofstream>(filePath, openMode);
    }
}

std::ostream &FileHelper::FileOrConsole::get() {
    if (ownedFile) {
        return *ownedFile;
    }
    return fallback;
}