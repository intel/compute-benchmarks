/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

namespace WorkloadInput {
template <typename ValueType>
struct BlackScholesOptions {
    std::vector<ValueType> years;
    std::vector<ValueType> strike;
    std::vector<ValueType> stockPrice;
};

template <typename ValueType>
inline BlackScholesOptions<ValueType> generateBlackScholesOptions(size_t optionCount) {
    constexpr uint32_t seed = 5347u;
    std::mt19937 generator(seed);
    std::uniform_real_distribution<ValueType> yearsDistribution(static_cast<ValueType>(0.25), static_cast<ValueType>(10.0));
    std::uniform_real_distribution<ValueType> strikeDistribution(static_cast<ValueType>(1.0), static_cast<ValueType>(100.0));
    std::uniform_real_distribution<ValueType> stockPriceDistribution(static_cast<ValueType>(5.0), static_cast<ValueType>(30.0));

    BlackScholesOptions<ValueType> options{};
    options.years.resize(optionCount);
    options.strike.resize(optionCount);
    options.stockPrice.resize(optionCount);
    for (size_t i = 0; i < optionCount; i++) {
        options.years[i] = yearsDistribution(generator);
        options.strike[i] = strikeDistribution(generator);
        options.stockPrice[i] = stockPriceDistribution(generator);
    }
    return options;
}

inline std::vector<uint32_t> generateGrayscaleImage(uint32_t width, uint32_t height) {
    constexpr uint32_t tileSize = 32u;
    constexpr uint32_t darkTile = 64u;
    constexpr uint32_t brightTile = 192u;
    constexpr uint32_t maxGradient = 63u;

    std::vector<uint32_t> image(static_cast<size_t>(width) * height);
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            const uint32_t tile = ((x / tileSize + y / tileSize) % 2u == 0u) ? darkTile : brightTile;
            const uint32_t gradient = (x + y) * maxGradient / (width + height);
            image[static_cast<size_t>(y) * width + x] = tile + gradient;
        }
    }
    return image;
}
} // namespace WorkloadInput
