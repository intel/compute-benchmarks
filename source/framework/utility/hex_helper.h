/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <iomanip>
#include <sstream>
#include <vector>

struct HexHelper {
    static std::string toHex(const std::vector<uint8_t> &bytes) {
        return toHex(bytes.data(), bytes.size());
    }

    static std::string toHex(const uint8_t *bytes, size_t bytesCount) {
        std::ostringstream output{};
        output << "0x";
        for (auto byteIndex = 0u; byteIndex < bytesCount; byteIndex++) {
            const uint8_t byte = bytes[byteIndex];
            output << std::uppercase << std::hex                      // print uppercase hex letters
                   << std::setfill('0') << std::setw(2) << std::right // always print 2 signs: 0x01 instead of 0x1
                   << static_cast<uint32_t>(byte);                    // cast to bigger type so it's not treated as char
        }
        return output.str();
    }

    static std::vector<uint8_t> fromHex(const std::string &hexString) {
        // Process prefix
        const std::string prefix = "0x";
        if (hexString.find(prefix) != 0) {
            return {};
        }

        // Each byte is two characters, so length should be even
        if (hexString.length() % 2 != 0) {
            return {};
        }

        // Iterate over bytes
        const auto bytesCount = hexString.length() / 2 - 1;
        std::vector<uint8_t> bytes = {};
        bytes.reserve(bytesCount);
        for (auto byteIndex = 0u; byteIndex < bytesCount; byteIndex++) {
            const auto byteStringOffset = prefix.length() + 2 * byteIndex;
            const auto byteString = hexString.substr(byteStringOffset, 2);
            const auto byte = static_cast<uint8_t>(std::stoul(byteString, nullptr, 16));
            bytes.push_back(byte);
        }
        return bytes;
    }
};
