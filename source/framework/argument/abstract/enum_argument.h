/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/argument.h"
#include "framework/utility/string_utils.h"

#include <cctype>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

template <typename T, typename = void>
struct HasNumberedValuesHelp : std::false_type {};

template <typename T>
struct HasNumberedValuesHelp<T, std::void_t<decltype(T::groupNumberedValuesInHelp)>> : std::bool_constant<T::groupNumberedValuesInHelp> {};

inline std::vector<std::string> groupNumberedNames(const char *const *names, size_t count) {
    constexpr size_t minimumGroupSize = 3;
    const auto split = [](const std::string &name) {
        size_t digitsStart = name.size();
        while (digitsStart > 0 && std::isdigit(static_cast<unsigned char>(name[digitsStart - 1]))) {
            digitsStart--;
        }
        const bool hasNumber = digitsStart < name.size() && digitsStart > 0;
        return std::make_pair(name.substr(0, hasNumber ? digitsStart : name.size()), hasNumber ? std::stoi(name.substr(digitsStart)) : -1);
    };

    std::vector<std::string> result{};
    size_t index = 0;
    while (index < count) {
        const auto [prefix, firstNumber] = split(names[index]);
        size_t end = index + 1;
        while (firstNumber >= 0 && end < count) {
            const auto [nextPrefix, nextNumber] = split(names[end]);
            if (nextPrefix != prefix || nextNumber != firstNumber + static_cast<int>(end - index)) {
                break;
            }
            end++;
        }
        if (end - index >= minimumGroupSize) {
            const int lastNumber = firstNumber + static_cast<int>(end - index) - 1;
            result.push_back(prefix + "X (X = " + std::to_string(firstNumber) + "-" + std::to_string(lastNumber) + ")");
        } else {
            for (size_t name = index; name < end; name++) {
                result.push_back(names[name]);
            }
        }
        index = end;
    }
    return result;
}

template <typename DerivedType, typename _EnumType>
struct EnumArgument : Argument {
    using EnumType = _EnumType;
    using ThisType = DerivedType;

    EnumArgument(ArgumentContainer &parent, const std::string &key) : EnumArgument(parent, key, "") {}
    EnumArgument(ArgumentContainer &parent, const std::string &key, const std::string &extraHelpPrefix)
        : Argument(parent, key, composeHelpMessage(extraHelpPrefix)) {
        static_assert(sizeof(DerivedType::enumValues) / sizeof(DerivedType::enumValues[0]) ==
                      sizeof(DerivedType::enumValuesNames) / sizeof(DerivedType::enumValuesNames[0]));
    }

    operator EnumType() const {
        return value;
    }

    static std::optional<EnumType> parseName(const std::string &name) {
        const std::string nameLower = toLower(name);
        const auto valuesCount = sizeof(DerivedType::enumValues) / sizeof(DerivedType::enumValues[0]);
        for (auto valueIndex = 0u; valueIndex < valuesCount; valueIndex++) {
            if (nameLower == toLower(DerivedType::enumValuesNames[valueIndex])) {
                return DerivedType::enumValues[valueIndex];
            }
        }
        return std::nullopt;
    }

    bool validate() const override {
        const auto valuesCount = sizeof(DerivedType::enumValues) / sizeof(DerivedType::enumValues[0]);
        for (auto valueIndex = 0u; valueIndex < valuesCount; valueIndex++) {
            if (this->value == DerivedType::enumValues[valueIndex]) {
                return true;
            }
        }
        return false;
    }

  protected:
    std::string toStringValue() const override {
        const auto valuesCount = sizeof(DerivedType::enumValues) / sizeof(DerivedType::enumValues[0]);
        for (auto valueIndex = 0u; valueIndex < valuesCount; valueIndex++) {
            if (this->value == DerivedType::enumValues[valueIndex]) {
                return DerivedType::enumValuesNames[valueIndex];
            }
        }
        FATAL_ERROR((std::string("Unknown ") + DerivedType::enumName));
    }

    void parseImpl(const std::string &valueToParse) override {
        this->value = parseName(valueToParse).value_or(DerivedType::invalidEnumValue);
    }

    EnumType value{};

  private:
    static std::string composeHelpMessage(const std::string &extraHelpPrefix) {
        std::ostringstream out{};

        if (extraHelpPrefix.size() > 0u) {
            out << extraHelpPrefix << " ";
        }

        out << "(";
        const auto enumValuesNamesCount = sizeof(DerivedType::enumValuesNames) / sizeof(DerivedType::enumValuesNames[0]);
        std::vector<std::string> names{};
        if constexpr (HasNumberedValuesHelp<DerivedType>::value) {
            names = groupNumberedNames(DerivedType::enumValuesNames, enumValuesNamesCount);
        } else {
            names.assign(DerivedType::enumValuesNames, DerivedType::enumValuesNames + enumValuesNamesCount);
        }
        for (auto i = 0u; i < names.size(); i++) {
            out << names[i];
            if (i != names.size() - 1) {
                out << " or ";
            }
        }
        out << ")";
        return out.str();
    }
};
