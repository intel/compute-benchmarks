/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

struct FileHelper {
    static std::vector<uint8_t> loadFile(const std::string &filePath, std::ios_base::openmode openMode);
    static std::vector<uint8_t> loadBinaryFile(const std::string &filePath);
    static std::vector<uint8_t> loadTextFile(const std::string &filePath);

    class FileOrConsole {
      public:
        FileOrConsole(const std::string &filePath, std::ios::openmode openMode, std::ostream &fallback);
        std::ostream &get();
        bool hasOwnedFile() const { return ownedFile != nullptr; }

      private:
        std::ostream &fallback;
        std::unique_ptr<std::ofstream> ownedFile = {};
    };
};
