/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "random_distribution.h"

#include <cassert>

UniformDistribution::UniformDistribution(size_t min, size_t max) : distr(min, max) {}

size_t UniformDistribution::get(std::mt19937 &gen) {
    return distr(gen);
}

LogUniformDistribution::LogUniformDistribution(size_t min, size_t max) {
    assert(min < max && min != 0);
    double quotient = (double)max / (double)min;
    minValue = min;
    maxValue = max;
    multiplier = (double)min;
    expotent = std::uniform_real_distribution<double>{0, std::log(quotient)};
}

size_t LogUniformDistribution::get(std::mt19937 &gen) {
    size_t result = static_cast<size_t>(multiplier * std::exp(expotent(gen)));
    // Check for numerical errors.
    if (result < minValue)
        return minValue;
    if (result > maxValue)
        return maxValue;
    return result;
}

std::unique_ptr<RandomDistribution> makeRandomDistribution(DistributionKind kind, size_t min, size_t max) {
    switch (kind) {
    case DistributionKind::Uniform:
        return std::make_unique<UniformDistribution>(min, max);
    case DistributionKind::LogUniform:
        return std::make_unique<LogUniformDistribution>(min, max);
    default:
        assert(false);
        return {}; // silence warning
    }
}
