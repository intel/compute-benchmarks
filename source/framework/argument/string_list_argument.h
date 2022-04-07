/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/argument.h"
#include "framework/utility/string_utils.h"

#include <vector>

struct StringListArgument : Argument {
    using Argument::Argument;

    operator const std::vector<std::string> &() const {
        return get();
    }

    const std::vector<std::string> &get() const {
        return value;
    }

    StringListArgument &operator=(const std::vector<std::string> &newValue) {
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
        FATAL_ERROR("ArgFilterArgument should not be printed");
    }

    void parseImpl(const std::string &valueToParse) override {
        for (const auto &filter : splitString(valueToParse)) {
            this->value.push_back(filter);
        }
    }

    std::vector<std::string> value = {};
    bool isValid = false;
};
