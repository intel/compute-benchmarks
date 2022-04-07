/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/argument.h"

struct BooleanFlagArgument : Argument {
    using Argument::Argument;

    operator bool() const {
        return value;
    }

    BooleanFlagArgument &operator=(bool newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    bool validate() const override {
        return isValid;
    }

  protected:
    std::string toStringValue() const override {
        return std::to_string(this->value);
    }

    void parseImpl(const std::string &valueToParse) override {
        this->value = true;
        this->isValid = valueToParse.empty();
    }

    std::string getHelpEntry(const std::string &argumentKey) const override {
        return std::string("--") + argumentKey;
    }

    bool value = false;
    bool isValid = true;
};
