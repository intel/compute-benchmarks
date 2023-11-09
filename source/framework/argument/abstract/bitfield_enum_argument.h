/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/argument.h"
#include "framework/utility/string_utils.h"

template <typename DerivedType, typename _EnumType>
struct BitfieldEnumArgument : Argument {
    constexpr static inline char separator = ':';
    using EnumType = _EnumType;
    using ThisType = DerivedType;

    BitfieldEnumArgument(ArgumentContainer &parent, const std::string &key) : BitfieldEnumArgument(parent, key, "") {}
    BitfieldEnumArgument(ArgumentContainer &parent, const std::string &key, const std::string &extraHelpPrefix)
        : Argument(parent, key, composeHelpMessage(extraHelpPrefix)) {
        static_assert(sizeof(DerivedType::enumValues) / sizeof(DerivedType::enumValues[0]) ==
                      sizeof(DerivedType::enumValuesNames) / sizeof(DerivedType::enumValuesNames[0]));
    }

    operator EnumType() const {
        return value;
    }

    bool validate() const override {
        size_t enabledValuesCount = 0u;

        const auto valuesCount = sizeof(DerivedType::enumValues) / sizeof(DerivedType::enumValues[0]);
        for (auto valueIndex = 0u; valueIndex < valuesCount; valueIndex++) {
            const auto currentValue = DerivedType::enumValues[valueIndex];
            if ((this->value & currentValue) == currentValue) {
                enabledValuesCount++;
            }
        }

        // At least one value must be enabled
        if (enabledValuesCount == 0) {
            return false;
        }

        // Incorrect values cannot by set
        const auto unknownValues = this->value & ~getAllValuesSum();
        if (unknownValues != DerivedType::zeroEnumValue) {
            return false;
        }

        if (!validateExtra()) {
            return false;
        }

        return true;
    }

  protected:
    std::string toStringValue() const override {
        std::ostringstream result{};

        bool hasValue = false;
        const auto valuesCount = sizeof(DerivedType::enumValues) / sizeof(DerivedType::enumValues[0]);
        for (auto valueIndex = 0u; valueIndex < valuesCount; valueIndex++) {
            const auto currentValue = DerivedType::enumValues[valueIndex];
            if ((this->value & currentValue) == currentValue) {
                if (hasValue) {
                    result << separator;
                }
                result << DerivedType::enumValuesNames[valueIndex];
                hasValue = true;
            }
        }

        FATAL_ERROR_UNLESS(hasValue, (std::string("Unknown ") + DerivedType::enumName));
        return result.str();
    }

    void parseImpl(const std::string &valueToParse) override {
        const std::string valueLower = toLower(valueToParse);

        size_t startIndex = 0;
        size_t endIndex = std::string::npos;
        this->value = DerivedType::zeroEnumValue;
        while (startIndex <= valueToParse.size()) {
            endIndex = valueToParse.find(separator, startIndex);
            if (endIndex == std::string::npos) {
                endIndex = valueToParse.size();
            }

            const std::string singleValue = valueLower.substr(startIndex, endIndex - startIndex);
            const EnumType parsedSingleValue = parseSingleValue(singleValue);
            if (parsedSingleValue == DerivedType::zeroEnumValue) {
                this->value = DerivedType::zeroEnumValue;
                return;
            }

            this->value = this->value | parsedSingleValue;

            startIndex = endIndex + 1;
        }
    }

  protected:
    EnumType value{};

  private:
    static EnumType parseSingleValue(const std::string &singleValueLower) {
        const auto valuesCount = sizeof(DerivedType::enumValues) / sizeof(DerivedType::enumValues[0]);
        for (auto valueIndex = 0u; valueIndex < valuesCount; valueIndex++) {
            if (singleValueLower == toLower(DerivedType::enumValuesNames[valueIndex])) {
                return DerivedType::enumValues[valueIndex];
            }
        }
        return DerivedType::zeroEnumValue;
    }

    static EnumType getAllValuesSum() {
        EnumType sum = DerivedType::zeroEnumValue;
        const auto valuesCount = sizeof(DerivedType::enumValues) / sizeof(DerivedType::enumValues[0]);
        for (auto valueIndex = 0u; valueIndex < valuesCount; valueIndex++) {
            const auto currentValue = DerivedType::enumValues[valueIndex];
            sum = sum | currentValue;
        }
        return sum;
    }

    static std::string composeHelpMessage(const std::string &extraHelpPrefix) {
        std::ostringstream out{};

        if (extraHelpPrefix.size() > 0u) {
            out << extraHelpPrefix << " ";
        }

        out << "(";
        const auto enumValuesNamesCount = sizeof(DerivedType::enumValuesNames) / sizeof(DerivedType::enumValuesNames[0]);
        for (auto i = 0u; i < enumValuesNamesCount; i++) {
            out << DerivedType::enumValuesNames[i];

            if (i != enumValuesNamesCount - 1) {
                out << " or ";
            }
        }
        out << " or a list separated with '" << separator << "')";
        return out.str();
    }

    virtual bool validateExtra() const { return true; }
};
