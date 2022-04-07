/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

inline bool parseArgumentToKeyValue(const std::string &argument, std::string &outKey, std::string &outValue) {
    // Check for common errors
    const std::string prefix = "--";
    const bool incorrectPrefix = argument.find(prefix) != 0;
    const bool empty = argument == prefix;
    if (incorrectPrefix || empty) {
        return false;
    }

    // Find '=', which is boundary between key and value. If it's not present, treat argument as a flag, which is also valid
    size_t index = argument.find('=');
    if (index == std::string::npos) {
        outKey = argument.substr(prefix.size());
        outValue = "";
        return true;
    }

    // Key-Value argument, also valid
    outKey = argument.substr(prefix.size(), index - prefix.size());
    outValue = argument.substr(index + 1);
    return true;
}

inline std::string toLower(const std::string &arg) {
    std::string result = arg;
    std::transform(arg.begin(), arg.end(), result.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

inline bool containsIllegalCharacters(const std::string &string, const std::string &illegalCharacters) {
    const auto predicate = [illegalCharacters](char c) {
        return illegalCharacters.find(c) != std::string::npos;
    };
    return std::any_of(string.begin(), string.end(), predicate);
}

inline std::vector<std::string> splitString(const std::string &string) {
    std::vector<std::string> result = {};
    std::istringstream stringStream(string);

    std::string token{};
    while (stringStream >> token) {
        result.push_back(token);
    }

    return result;
}

inline std::pair<std::string_view, bool> handleFilterNegation(std::string_view string) {
    bool negated = false;
    while (!string.empty() && string.front() == '^') {
        string.remove_prefix(1);
        negated = !negated;
    }
    return {string, negated};
}

inline bool endsWith(const std::string &string, const std::string &ending) {
    if (string.length() < ending.length()) {
        return false;
    }

    return (0 == string.compare(string.length() - ending.length(), ending.length(), ending));
}

inline size_t cutLeadingSpaces(std::string &string) {
    size_t result = {};
    for (char c : string) {
        if (c != ' ') {
            break;
        }
        result++;
    }
    string = string.substr(result);
    return result;
}

template <typename T>
using ToStringConverter = std::string (*)(T);

template <typename T>
inline std::string joinStrings(const std::string &separator, const std::vector<T> &objects, ToStringConverter<T> toString) {
    std::ostringstream result{};
    for (auto i = 0u; i < objects.size(); i++) {
        result << toString(objects[i]);
        if (i < objects.size() - 1) {
            result << separator;
        }
    }
    return result.str();
}

inline std::string indentString(const std::string &string, size_t howManySpaces) {
    const std::string indent = std::string(howManySpaces, ' ');
    return indent + std::regex_replace(string, std::regex("(\n)[^^]"), std::string("\n") + indent);
}
