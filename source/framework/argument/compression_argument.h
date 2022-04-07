/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/configuration.h"

struct CompressionBooleanArgument : BooleanArgument {
    using BooleanArgument::BooleanArgument;

    CompressionBooleanArgument &operator=(bool newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

  protected:
    std::string toStringValue() const override {
        if (Configuration::get().noIntelExtensions) {
            return "?";
        }
        return BooleanArgument::toStringValue();
    }
};
