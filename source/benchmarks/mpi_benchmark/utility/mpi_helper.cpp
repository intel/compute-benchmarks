/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "mpi_helper.h"

#include <array>
#include <gtest/gtest.h>
#include <unistd.h>

std::vector<std::chrono::nanoseconds> runLauncher(const std::string &launcher, const int nRanks, const std::string &binary, const std::string &args, const int nIters) {
    std::array<char, 512> exeFile{};
    EXPECT_NE(0, readlink("/proc/self/exe", exeFile.data(), exeFile.size()));
    std::string workingDir = exeFile.data();
    workingDir = workingDir.substr(0, workingDir.find_last_of('/') + 1);

    const std::string mpiCommand = launcher + " -n " + std::to_string(nRanks) + ' ' + workingDir + binary + ' ' + args;

    FILE *pipe = nullptr;
    std::array<char, 512> stdoutLine{};
    std::vector<std::chrono::nanoseconds> measurements{};

    for (int i = 0; i < nIters; i++) {
        std::vector<std::string> output{};
        pipe = popen(mpiCommand.c_str(), "r");
        EXPECT_NE(nullptr, pipe);
        while (fgets(stdoutLine.data(), stdoutLine.size(), pipe) != nullptr) {
            output.push_back(stdoutLine.data());
        }
        EXPECT_EQ(0, pclose(pipe));

        bool foundMeasurement = false;
        for (auto &line : output) {
            const auto pos = line.rfind("**Measurement:", 0);
            if (pos == 0) {
                measurements.emplace_back(std::stoull(line.substr(line.find_last_of("**Measurement:") + 1)));
                foundMeasurement = true;
                break;
            }
        }
        // Early termination
        if (!foundMeasurement) {
            return measurements;
        }
    }

    return measurements;
}
