/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/argument.h"

#include <bitset>

template <size_t bitsCount, bool canBeAllZeros>
struct BitmaskArgument : Argument {
    using Argument::Argument;
    using Bitset = std::bitset<bitsCount>;

    operator Bitset() const {
        return value;
    }

    BitmaskArgument &operator=(Bitset newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    std::vector<size_t> getEnabledBits() const {
        std::vector<size_t> result{};
        result.reserve(this->value.count());
        for (auto bitIndex = 0u; bitIndex < bitsCount; bitIndex++) {
            if (this->value.test(bitIndex)) {
                result.push_back(bitIndex);
            }
        }
        return result;
    }

    bool validate() const override {
        if constexpr (!canBeAllZeros) {
            if (this->value.none()) {
                return false;
            }
        }
        return this->valid;
    }

  protected:
    std::string toStringValue() const override {
        return value.to_string();
    }

    void parseImpl(const std::string &valueToParse) override {
        this->valid = false;

        if (valueToParse.size() > bitsCount) {
            return;
        }
        if (valueToParse.size() == 0) {
            return;
        }
        const auto isInvalidCharacter = [](char c) { return c != '0' && c != '1'; };
        if (std::any_of(valueToParse.begin(), valueToParse.end(), isInvalidCharacter)) {
            return;
        }

        this->value = Bitset(valueToParse);
        if constexpr (!canBeAllZeros) {
            if (this->value.none()) {
                return;
            }
        }

        this->valid = true;
    }

    Bitset value = {};
    bool valid = true;
};
