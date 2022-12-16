/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/argument.h"
#include "framework/utility/string_utils.h"

#include <vector>

struct StringArgument : Argument {
    using Argument::Argument;

    operator const std::string &() const {
        return value;
    }

    StringArgument &operator=(const std::string &newValue) {
        this->value = newValue;
        this->isValid = true;
        markAsParsed();
        return *this;
    }

    bool validate() const override {
        return isValid;
    }

  protected:
    std::string toStringValue() const override {
        return value;
    }

    void parseImpl(const std::string &valueToParse) override {
        this->value = valueToParse;
        this->isValid = true;
    }

    std::string value = {};
    bool isValid = false;
};
