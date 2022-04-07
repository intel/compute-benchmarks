/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/argument.h"

#include <tuple>

struct ThreeComponentUintArgument : Argument {
    using Argument::Argument;
    using TupleType = std::tuple<size_t, size_t, size_t>;

    operator const size_t *() const {
        return value;
    }

    ThreeComponentUintArgument &operator=(TupleType newValue) {
        assign(newValue);
        markAsParsed();
        return *this;
    }

  protected:
    void assign(TupleType newValue) {
        this->value[0] = std::get<0>(newValue);
        this->value[1] = std::get<1>(newValue);
        this->value[2] = std::get<2>(newValue);
    }

    std::string toStringValue() const override {
        std::ostringstream result{};
        result << value[0] << ":"
               << value[1] << ":"
               << value[2];
        return result.str();
    }

    void parseImpl(const std::string &valueToParse) override {
        const auto colonPos1 = valueToParse.find(":");
        const auto colonPos2 = valueToParse.find(":", colonPos1 + 1);
        const auto colonPos3 = valueToParse.find(":", colonPos2 + 1);
        FATAL_ERROR_IF(colonPos1 == std::string::npos, "Too few colons specified for a 3-component vector");
        FATAL_ERROR_IF(colonPos2 == std::string::npos, "Too few colons specified for a 3-component vector");
        FATAL_ERROR_IF(colonPos3 != std::string::npos, "Too many colons specified for a 3-component vector");

        const std::string componentsString[3] = {
            valueToParse.substr(0, colonPos1),
            valueToParse.substr(colonPos1 + 1, colonPos2 - colonPos1 - 1),
            valueToParse.substr(colonPos2 + 1),
        };

        this->value[0] = std::atoi(componentsString[0].c_str());
        this->value[1] = std::atoi(componentsString[1].c_str());
        this->value[2] = std::atoi(componentsString[2].c_str());
    }

    size_t value[3] = {0, 0, 0};
};

struct ThreeComponentOffsetArgument : ThreeComponentUintArgument {
    using ThreeComponentUintArgument::ThreeComponentUintArgument;

    ThreeComponentOffsetArgument &operator=(TupleType newValue) {
        assign(newValue);
        return *this;
    }
};

struct ThreeComponentSizeArgument : ThreeComponentUintArgument {
    using ThreeComponentUintArgument::ThreeComponentUintArgument;

    ThreeComponentSizeArgument &operator=(TupleType newValue) {
        assign(newValue);
        return *this;
    }

    bool validate() const override {
        return value[0] > 0 &&
               value[1] > 0 &&
               value[2] > 0;
    }
};
