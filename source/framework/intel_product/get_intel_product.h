/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <cstdint>
#include <string>

// clang-format off
#define INTEL_GEN(GEN) GEN,
enum class IntelGen {
    #include "framework/intel_product/intel_products.inl"
};
#undef INTEL_GEN

#define INTEL_PRODUCT(PRODUCT, GEN) PRODUCT,
enum class IntelProduct {
     #include "framework/intel_product/intel_products.inl"
};
#undef INTEL_PRODUCT

#define INTEL_PRODUCT_ID(DEVICE_ID, PRODUCT) case DEVICE_ID: return IntelProduct::PRODUCT;
inline IntelProduct getIntelProduct(uint32_t deviceId) {
    switch (deviceId) {
        #include "framework/intel_product/intel_products.inl"
        default: return IntelProduct::Unknown;
    }
}
#undef INTEL_PRODUCT_ID

#define INTEL_PRODUCT(PRODUCT, GEN) case IntelProduct::PRODUCT: return IntelGen::GEN;
inline IntelGen getIntelGen(IntelProduct product) {
    switch (product) {
        #include "framework/intel_product/intel_products.inl"
        default: return IntelGen::Unknown;
    }
}
#undef INTEL_PRODUCT

template <typename Arg>
inline IntelGen getIntelGen(Arg &&arg) {
    return getIntelGen(getIntelProduct(std::forward<Arg>(arg)));
}

#define INTEL_PRODUCT(PRODUCT, GEN) case IntelProduct::PRODUCT: return #PRODUCT;
namespace std {
    inline std::string to_string(IntelProduct product) {
        switch (product) {
            #include "framework/intel_product/intel_products.inl"
            default: return "Unknown";
        }
    }
}
#undef INTEL_PRODUCT

#define INTEL_GEN(GEN) case IntelGen::GEN: return #GEN;
namespace std {
    inline std::string to_string(IntelGen gen) {
        switch (gen) {
            #include "framework/intel_product/intel_products.inl"
            default: return "Unknown";
        }
    }
}
#undef INTEL_GEN

// clang-format on
