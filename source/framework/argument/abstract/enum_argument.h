/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/argument.h"
#include "framework/utility/string_utils.h"

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
        const std::string valueToParseLower = toLower(valueToParse);
        const auto valuesCount = sizeof(DerivedType::enumValues) / sizeof(DerivedType::enumValues[0]);
        for (auto valueIndex = 0u; valueIndex < valuesCount; valueIndex++) {
            if (valueToParseLower == toLower(DerivedType::enumValuesNames[valueIndex])) {
                this->value = DerivedType::enumValues[valueIndex];
                return;
            }
        }
        this->value = DerivedType::invalidEnumValue;
    }

    EnumType value;

  private:
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
        out << ")";
        return out.str();
    }
};
