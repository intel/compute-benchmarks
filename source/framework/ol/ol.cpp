/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "ol.h"

#include "error.h"

namespace {
struct OlDeviceSelection {
    size_t requestedIndex;
    size_t currentIndex = 0;
    ol_device_handle_t device = nullptr;
};

bool selectDevice(ol_device_handle_t device, void *userData) {
    auto &selection = *static_cast<OlDeviceSelection *>(userData);
    if (selection.currentIndex == selection.requestedIndex) {
        selection.device = device;
        return false;
    }

    ++selection.currentIndex;
    return true;
}

bool selectHostDevice(ol_device_handle_t device, void *userData) {
    ol_device_type_t type{};
    if (olGetDeviceInfo(device, OL_DEVICE_INFO_TYPE, sizeof(type), &type) !=
        OL_SUCCESS) {
        return true;
    }
    if (type == OL_DEVICE_TYPE_HOST) {
        ol_device_handle_t *hostDevicePtr =
            static_cast<ol_device_handle_t *>(userData);
        *hostDevicePtr = device;
        return false;
    }
    return true;
}
} // namespace

OlInitGuard::OlInitGuard(ol_init_args_t *initArgs) {
    OL_RESULT_SUCCESS_OR_ERROR(olInit(initArgs));
}

OlInitGuard::~OlInitGuard() {
    EXPECT_OL_RESULT_SUCCESS(olShutDown());
}

OlState::OlState() : olInitGuard(nullptr) {
    OlDeviceSelection selection{Configuration::get().olDeviceIndex};
    OL_RESULT_SUCCESS_OR_ERROR(olIterateDevices(selectDevice, &selection));

    if (!selection.device) {
        FATAL_ERROR("Invalid liboffload device index. deviceIndex=",
                    Configuration::get().olDeviceIndex, " deviceCount=",
                    selection.currentIndex);
    }
    device = selection.device;

    OL_RESULT_SUCCESS_OR_ERROR(olIterateDevices(selectHostDevice, &hostDevice));

    OL_RESULT_SUCCESS_OR_ERROR(olCreateContext(1, &device, &context));
}

OlState::~OlState() {
    if (context) {
        EXPECT_OL_RESULT_SUCCESS(olDestroyContext(context));
    }
}
