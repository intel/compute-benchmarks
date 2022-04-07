/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/utility/command_line_argument.h"

#include <string>

class BenchmarkMain {
  public:
    BenchmarkMain(int argc, char **argv, const std::string benchmarkVersion);
    int main();

  private:
    int argc;
    char **argv;
    const std::string benchmarkVersion;
    CommandLineArguments commandLineArguments = {};

    int setupEnvironment();

    int printVersion(bool enableWarning, const char *prefix = "");
    void printHelp();
    int generateDocs();

    int executeSingleTest(const std::string &testName);
    int executeAllTests();
};
