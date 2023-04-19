/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/sycl/sycl.h"

#include <iomanip>
#include <map>
#include <string>
#include <vector>

namespace SYCL {

std::map<sycl::info::device_type, std::string> deviceTypeToString{
    {sycl::info::device_type::cpu, "CPU"},
    {sycl::info::device_type::gpu, "GPU"},
    {sycl::info::device_type::accelerator, "Accelerator"},
    {sycl::info::device_type::custom, "Custom"},
    {sycl::info::device_type::automatic, "Automatic"},
    {sycl::info::device_type::host, "Host"},
    {sycl::info::device_type::all, "All"}};

std::map<sycl::backend, std::string> backendToString{
    {sycl::backend::opencl, "OpenCL"},
    {sycl::backend::ext_oneapi_level_zero, "oneAPI Level Zero"},
    {sycl::backend::ext_oneapi_cuda, "oneAPI CUDA"},
    {sycl::backend::all, "All"},
    {sycl::backend::ext_intel_esimd_emulator, "ESIMD CPU"}};

void printDeviceInfo() {
    auto device = sycl::device{sycl::default_selector_v};
    auto platform = device.get_platform();

    auto driverVersion = device.get_info<sycl::info::device::driver_version>();
    auto version = device.get_info<sycl::info::device::version>();
    auto deviceName = device.get_info<sycl::info::device::name>();
    auto vendorName = device.get_info<sycl::info::device::vendor>();
    auto deviceType = deviceTypeToString[device.get_info<sycl::info::device::device_type>()];
    auto backend = platform.get_backend();

    std::cout << "Using SYCL backend: " << backendToString[backend] << std::endl;
    std::cout << "Driver version: " << driverVersion << std::endl;

    if (backend == sycl::backend::opencl) {
        std::cout << "OpenCL C version: " << device.get_info<sycl::info::device::opencl_c_version>() << std::endl;
    }
    std::cout << "\tDevice: " << deviceName << std::endl;
    std::cout << "\t\tVendor:\t" << vendorName << std::endl;
    std::cout << "\t\tType:\t" << deviceType << std::endl;
    std::cout << "\t\tVersion:\t" << version << std::endl;

    std::cout << std::endl;
}

static void printAvailableDevices() {
    std::vector<sycl::device> devices = sycl::device::get_devices();

    if (devices.size() == 0) {
        std::cout << "SYCL devices: NONE";
        return;
    }

    std::cout << "SYCL devices: " << devices.size() << std::endl;

    for (auto &device : devices) {
        auto driverVersion = device.get_info<sycl::info::device::driver_version>();
        auto deviceName = device.get_info<sycl::info::device::name>();
        auto vendorName = device.get_info<sycl::info::device::vendor>();
        auto deviceType = deviceTypeToString[device.get_info<sycl::info::device::device_type>()];
        auto version = device.get_info<sycl::info::device::version>();
        auto backend = device.get_backend();
        std::cout << "\tDevice: " << deviceName << std::endl;
        std::cout << "\t\tType:\t" << deviceType << std::endl;
        std::cout << "\t\tSYCL Backend:\t" << backendToString[backend] << std::endl;
        std::cout << "\t\tVendor:\t" << vendorName << std::endl;
        std::cout << "\t\tDriver version:\t" << driverVersion << std::endl;
        std::cout << "\t\tVersion:\t" << version << std::endl;
        if (backend == sycl::backend::opencl) {
            std::cout << "\t\tOpenCL C Version:\t" << device.get_info<sycl::info::device::opencl_c_version>() << std::endl;
        }

        std::cout << std::endl;
    }
    std::cout << std::endl;
}

} // namespace SYCL
