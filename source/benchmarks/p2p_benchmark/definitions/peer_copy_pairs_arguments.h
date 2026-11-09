/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/engine_argument.h"
#include "framework/argument/string_argument.h"
#include "framework/test_case/test_case_argument_container.h"
#include "framework/utility/common_help_message.h"
#include "framework/utility/string_utils.h"

#include <algorithm>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

struct PeerCopyPairsArguments : TestCaseArgumentContainer {
    static constexpr char pairSeparator = '_';
    static constexpr char deviceSeparator = ':';
    static constexpr char engineSeparator = '_';
    static constexpr size_t maxDeviceIdDigits = 4;

    StringArgument pairs;
    ByteSizeArgument size;
    StringArgument engines;
    BooleanArgument useEvents;

    PeerCopyPairsArguments()
        : pairs(*this, "pairs", "Pairs of root devices as local:peer, separated by _, for example 0:1_0:2_0:3. The local device of every pair submits its copy. At least two device pairs, no pair given twice"),
          size(*this, "size", "Size of each copy"),
          engines(*this, "engines", "Engines of the device that submits the copies, separated by _, for example BCS0_BCS1_CCS0. The first copy of a device runs on the first engine, its second copy on the second engine, and so on, so a device needs one engine for every copy it submits. No engine given twice"),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()) {}

    bool validateArgumentsExtra() const override {
        const auto parsedPairs = parsePairs(this->pairs);
        const auto parsedEngines = parseEngines(this->engines);
        if (!parsedPairs || !parsedEngines) {
            return false;
        }
        for (const auto &[localDeviceId, peerDeviceId] : *parsedPairs) {
            const auto copiesOfDevice = std::count_if(parsedPairs->begin(), parsedPairs->end(), [localDeviceId = localDeviceId](const auto &pair) { return pair.first == localDeviceId; });
            if (static_cast<size_t>(copiesOfDevice) > parsedEngines->size()) {
                return false;
            }
        }
        return true;
    }

    std::vector<std::pair<size_t, size_t>> getPairs() const {
        return parsePairs(this->pairs).value_or(std::vector<std::pair<size_t, size_t>>{});
    }

    std::vector<Engine> getEngines() const {
        return parseEngines(this->engines).value_or(std::vector<Engine>{});
    }

    static std::optional<std::vector<Engine>> parseEngines(const std::string &text) {
        constexpr size_t engineCount = sizeof(EngineArgument::enumValues) / sizeof(EngineArgument::enumValues[0]);
        std::vector<Engine> result;
        size_t start = 0;
        while (start <= text.size()) {
            const size_t end = std::min(text.find(engineSeparator, start), text.size());
            const std::string name = toLower(text.substr(start, end - start));
            const auto *const nameEnd = EngineArgument::enumValuesNames + engineCount;
            const auto *const match = std::find_if(EngineArgument::enumValuesNames, nameEnd, [&name](const char *engineName) { return name == toLower(engineName); });
            if (match == nameEnd) {
                return std::nullopt;
            }
            const Engine engine = EngineArgument::enumValues[match - EngineArgument::enumValuesNames];
            if (std::find(result.begin(), result.end(), engine) != result.end()) {
                return std::nullopt;
            }
            result.push_back(engine);
            start = end + 1;
        }
        return result;
    }

    static std::optional<size_t> parseDeviceId(const std::string &text) {
        if (text.empty() || text.size() > maxDeviceIdDigits || text.find_first_not_of("0123456789") != std::string::npos) {
            return std::nullopt;
        }
        return static_cast<size_t>(std::stoul(text));
    }

    static std::optional<std::vector<std::pair<size_t, size_t>>> parsePairs(const std::string &text) {
        std::vector<std::pair<size_t, size_t>> result;
        size_t start = 0;
        while (start <= text.size()) {
            const size_t end = std::min(text.find(pairSeparator, start), text.size());
            const std::string pair = text.substr(start, end - start);
            const size_t separator = pair.find(deviceSeparator);
            if (separator == std::string::npos) {
                return std::nullopt;
            }
            const auto localDeviceId = parseDeviceId(pair.substr(0, separator));
            const auto peerDeviceId = parseDeviceId(pair.substr(separator + 1));
            if (!localDeviceId || !peerDeviceId || *localDeviceId == *peerDeviceId) {
                return std::nullopt;
            }
            result.emplace_back(*localDeviceId, *peerDeviceId);
            start = end + 1;
        }
        std::set<std::pair<size_t, size_t>> copies;
        std::set<std::pair<size_t, size_t>> devicePairs;
        for (const auto &[localDeviceId, peerDeviceId] : result) {
            if (!copies.emplace(localDeviceId, peerDeviceId).second) {
                return std::nullopt;
            }
            devicePairs.emplace(std::min(localDeviceId, peerDeviceId), std::max(localDeviceId, peerDeviceId));
        }
        if (devicePairs.size() < 2) {
            return std::nullopt;
        }
        return result;
    }
};
