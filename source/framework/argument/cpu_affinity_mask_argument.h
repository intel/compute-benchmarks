/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/argument.h"

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <sstream>

// CPU affinity mask. The mask is stored in a uint64_t, so at most 64 CPUs (bit i
// = CPU i) can be addressed; this is a storage limit of this argument, not of the
// OS scheduler (Linux cpu_set_t and Windows processor groups both go higher). On
// Windows SetProcessAffinityMask is relative to the process's current processor
// group, so bit i selects the i-th CPU within that group. Accepts either a number
// (decimal or 0x-prefixed hex, e.g. 255, 0xff00) or a CPU list (e.g. 0,2,4-7).
// Value 0 means "not set".
struct CpuAffinityMaskArgument : Argument {
    using Argument::Argument;
    static constexpr uint64_t maxCpuCount = 64u;

    operator uint64_t() const {
        return value;
    }

    CpuAffinityMaskArgument &operator=(uint64_t newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    bool validate() const override {
        return this->valid;
    }

  protected:
    std::string toStringValue() const override {
        std::ostringstream result;
        result << "0x" << std::hex << this->value;
        return result.str();
    }

    void parseImpl(const std::string &valueToParse) override {
        this->value = 0u;
        const bool isCpuList = valueToParse.find_first_of(",-") != std::string::npos;
        this->valid = isCpuList ? parseCpuList(valueToParse) : parseNumber(valueToParse);
        if (!this->valid) {
            std::cerr << "Invalid cpuAffinityMask \"" << valueToParse << "\": expected a bitmask (decimal or 0x-prefixed hex) "
                      << "or a list of CPU indices 0-" << (maxCpuCount - 1) << " (e.g. 0,2,4-7)\n";
        }
    }

    bool parseNumber(const std::string &valueToParse) {
        if (valueToParse.empty()) {
            return false;
        }

        errno = 0;
        char *end = nullptr;
        const unsigned long long parsedMask = std::strtoull(valueToParse.c_str(), &end, 0);
        if (end == valueToParse.c_str() || *end != '\0' || errno == ERANGE) {
            return false;
        }

        this->value = static_cast<uint64_t>(parsedMask);
        return true;
    }

    bool parseCpuList(const std::string &valueToParse) {
        uint64_t mask = 0u;
        for (size_t tokenStart = 0u; tokenStart <= valueToParse.size();) {
            const size_t commaPosition = valueToParse.find(',', tokenStart);
            const std::string token = valueToParse.substr(tokenStart, commaPosition - tokenStart);
            const size_t dashPosition = token.find('-');

            uint64_t firstCpu = 0u;
            uint64_t lastCpu = 0u;
            if (dashPosition == std::string::npos) {
                if (!parseCpuIndex(token, firstCpu)) {
                    return false;
                }
                lastCpu = firstCpu;
            } else {
                if (!parseCpuIndex(token.substr(0, dashPosition), firstCpu) ||
                    !parseCpuIndex(token.substr(dashPosition + 1), lastCpu) ||
                    firstCpu > lastCpu) {
                    return false;
                }
            }

            for (uint64_t cpu = firstCpu; cpu <= lastCpu; cpu++) {
                mask |= 1ull << cpu;
            }

            if (commaPosition == std::string::npos) {
                break;
            }
            tokenStart = commaPosition + 1;
        }

        this->value = mask;
        return true;
    }

    static bool parseCpuIndex(const std::string &token, uint64_t &outCpu) {
        const auto isNotDigit = [](char c) { return c < '0' || c > '9'; };
        if (token.empty() || std::any_of(token.begin(), token.end(), isNotDigit)) {
            return false;
        }

        errno = 0;
        const unsigned long long parsedCpu = std::strtoull(token.c_str(), nullptr, 10);
        if (errno == ERANGE || parsedCpu >= maxCpuCount) {
            return false;
        }

        outCpu = static_cast<uint64_t>(parsedCpu);
        return true;
    }

    uint64_t value = 0u;
    bool valid = true;
};
