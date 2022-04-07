/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/argument.h"
#include "framework/utility/string_utils.h"

struct IntegerArgumentBase : Argument {
    using Argument::Argument;

    int64_t getSizeOf() const {
        return sizeof(value);
    }

    const int64_t *getAddressOf() const {
        return &value;
    }

  protected:
    std::string toStringValue() const override {
        return std::to_string(this->value);
    }

    void parseImpl(const std::string &valueToParse) override {
        this->value = std::atoi(valueToParse.c_str());
    }

    int64_t value = 0u;
};

struct IntegerArgument : IntegerArgumentBase {
    using IntegerArgumentBase::IntegerArgumentBase;

    operator int64_t() const {
        return value;
    }

    IntegerArgument &operator=(int64_t newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }
};

struct PositiveIntegerArgument : IntegerArgumentBase {
    using IntegerArgumentBase::IntegerArgumentBase;

    operator size_t() const {
        return static_cast<size_t>(value);
    }

    PositiveIntegerArgument &operator=(size_t newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    bool validate() const override {
        return this->value > 0;
    }
};

struct NonNegativeIntegerArgument : IntegerArgumentBase {
    using IntegerArgumentBase::IntegerArgumentBase;

    operator size_t() const {
        return static_cast<size_t>(value);
    }

    NonNegativeIntegerArgument &operator=(size_t newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    bool validate() const override {
        return this->value >= 0;
    }
};

struct ByteSizeArgument : PositiveIntegerArgument {
    using PositiveIntegerArgument::PositiveIntegerArgument;

    ByteSizeArgument &operator=(size_t newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    std::string toStringValue() const override {
        if (this->value == 0) {
            return "0";
        }

        const std::string units[] = {"", "KB", "MB", "GB"};
        const auto unitCount = sizeof(units) / sizeof(units[0]);

        auto currentUnit = 0u;
        auto currentValue = this->value;
        for (; currentUnit < unitCount; currentUnit++) {
            if (currentValue % 1024 != 0) {
                break;
            }
            currentValue /= 1024;
        }

        return std::to_string(currentValue) + units[currentUnit];
    }

    void parseImpl(const std::string &valueToParse) override {
        const std::string units[] = {"kb", "mb", "gb", "b", ""};
        const size_t unitMultipliers[] = {1024, 1024 * 1024, 1024 * 1024 * 1024, 1, 1};
        const auto unitCount = sizeof(units) / sizeof(units[0]);
        static_assert(unitCount == sizeof(unitMultipliers) / sizeof(unitMultipliers[0]));

        const std::string valueToParseLower = toLower(valueToParse);
        std::string valueWithoutUnit{};
        auto currentUnit = 0u;
        for (; currentUnit < unitCount; currentUnit++) {
            if (valueToParseLower.length() < units[currentUnit].length()) {
                continue;
            }

            const auto unitPosition = valueToParseLower.rfind(units[currentUnit]);
            const auto expectedUnitPosition = valueToParseLower.length() - units[currentUnit].length();
            if (unitPosition == expectedUnitPosition) {
                valueWithoutUnit = valueToParseLower.substr(0, unitPosition);
                break;
            }
        }

        this->value = std::atoi(valueWithoutUnit.c_str());
        this->value *= unitMultipliers[currentUnit];
    }
};

struct BooleanArgument : Argument {
    BooleanArgument(ArgumentContainer &parent, const std::string &key, const std::string &extraHelp)
        : Argument(parent, key, extraHelp + " (0 or 1)") {}
    BooleanArgument(ArgumentContainer &parent, const std::string &key)
        : Argument(parent, key, "(0 or 1)") {}

    operator bool() const {
        return value != 0;
    }

    BooleanArgument &operator=(bool newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    bool validate() const override {
        return value == 0 || value == 1;
    }

  protected:
    std::string toStringValue() const override {
        return std::to_string(this->value);
    }

    void parseImpl(const std::string &valueToParse) override {
        this->value = std::atoi(valueToParse.c_str());
    }

    int value = -1;
};
