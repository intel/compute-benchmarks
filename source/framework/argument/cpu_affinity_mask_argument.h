/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/argument.h"
#include "framework/utility/cpu_topology.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>

// CPU affinity mask (bit i = CPU i), limited to 64 CPUs. On Windows, bit i is
// the i-th CPU in the process's current processor group, not global CPU i.
//
// Accepts a number (decimal or 0x-hex), a CPU list (0,2,4-7), or on Intel
// hybrid CPUs a core-type keyword ("p-cores"/"e-cores", resolved via CPUID).
// A non-hybrid CPU makes the keyword a no-op; a hybrid CPU with none of the
// requested type is an error. 0 means "not set".
struct CpuAffinityMaskArgument : Argument {
    using Argument::Argument;
    static constexpr uint64_t maxCpuCount = CpuTopologyDetail::maxDetectableCpuCount;

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

        if (const auto coreType = parseCoreTypeKeyword(valueToParse)) {
            std::string errorMessage{};
            uint64_t mask = 0u;
            switch (getCpuMaskForCoreType(*coreType, mask, errorMessage)) {
            case CoreTypeLookupResult::Success:
                this->value = mask;
                this->valid = true;
                break;
            case CoreTypeLookupResult::NotHybrid:
                // Also on stdout: captured output of a p-cores and an e-cores run on the same non-hybrid machine must differ.
                std::cerr << "cpuAffinityMask \"" << valueToParse << "\" ignored: " << errorMessage << '\n';
                std::cout << "CPU affinity: not pinned, \"" << valueToParse << "\" ignored on this machine" << std::endl;
                this->value = 0u;
                this->valid = true;
                break;
            case CoreTypeLookupResult::NoMatchingCpu:
                this->value = 0u;
                this->valid = false;
                std::cerr << "Invalid cpuAffinityMask \"" << valueToParse << "\": " << errorMessage << '\n';
                break;
            }
            return;
        }

        const bool isCpuList = valueToParse.find_first_of(",-") != std::string::npos;
        this->valid = isCpuList ? parseCpuList(valueToParse) : parseNumber(valueToParse);
        if (!this->valid) {
            std::cerr << "Invalid cpuAffinityMask \"" << valueToParse << "\": expected a bitmask (decimal or 0x-prefixed hex), "
                      << "a list of CPU indices 0-" << (maxCpuCount - 1) << " (e.g. 0,2,4-7), "
                      << "or a core-type keyword (p-cores, e-cores)\n";
        }
    }

    static std::optional<CpuCoreType> parseCoreTypeKeyword(const std::string &valueToParse) {
        std::string normalized;
        normalized.reserve(valueToParse.size());
        for (const char character : valueToParse) {
            if (character == '-' || character == '_') {
                continue;
            }
            normalized += static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
        }

        if (normalized == "pcores") {
            return CpuCoreType::Performance;
        }
        if (normalized == "ecores") {
            return CpuCoreType::Efficiency;
        }
        return std::nullopt;
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
