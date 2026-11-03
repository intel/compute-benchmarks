/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/test_case/test_result.h"

#include <iostream>
#include <sycl/ext/oneapi/experimental/graph.hpp>
#include <sycl/sycl.hpp>

namespace GraphHelpers {
inline TestResult checkNativeRecordingSupport(const sycl::queue &queue) {
    namespace sycl_ext = sycl::ext::oneapi::experimental;
    try {
        sycl_ext::command_graph graph{queue.get_context(), queue.get_device(),
                                      sycl::property_list{sycl_ext::property::graph::enable_native_recording{}}};
    } catch (const sycl::exception &e) {
        if (e.code() == sycl::errc::invalid || e.code() == sycl::errc::feature_not_supported) {
            return TestResult::DeviceNotCapable;
        }
        std::cerr << "Caught SYCL exception: " << e.what() << "\n";
        return TestResult::Error;
    }
    return TestResult::Success;
}
} // namespace GraphHelpers
