/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "sycl.h"

namespace SYCL {
Sycl::Sycl() : device(),
               queue(device) {}

Sycl::Sycl(const sycl::device_selector &deviceSelector) : device(deviceSelector),
                                                          queue(device) {}

Sycl::Sycl(const sycl::property_list &propertyList) : device(),
                                                      queue(device, propertyList) {}

Sycl::Sycl(const sycl::device_selector &deviceSelector, const sycl::property_list &propertyList) : device(deviceSelector),
                                                                                                   queue(propertyList) {}

Sycl::~Sycl() {}
} // namespace SYCL
