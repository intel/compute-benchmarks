/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/utility/hex_helper.h"

struct LongHexArgument : Argument {
    using Argument::Argument;

    operator const std::vector<uint8_t> &() const {
        return bytes;
    }

    LongHexArgument &operator=(const std::string &hex) {
        parseImpl(hex);
        markAsParsed();
        return *this;
    }

    bool validate() const override {
        return bytes.size() > 0;
    }

  protected:
    std::string toStringValue() const override {
        return HexHelper::toHex(bytes);
    }

    void parseImpl(const std::string &value) override {
        bytes = HexHelper::fromHex(value);
    }

    std::vector<uint8_t> bytes;
};
