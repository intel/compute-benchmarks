/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/ocl/context_properties.h"
#include "framework/ocl/function_signatures_ocl.h"
#include "framework/ocl/queue_properties.h"
#include "framework/ocl/utility/error.h"
#include "framework/ocl/utility/extensions_helper.h"
#include "framework/test_case/test_case.h"

namespace OCL {

// Class handles regular OpenCL boilerplate code, such as querying driver handle, devices, creating contexts,
// and queues. It is configurable by the QueueProperties and ContextProperties objects, allowing to perform the
// setup in a different way than usual. Default constructor always creates both the context and the queue.
//
// Opencl performs it's own cleanup.
struct Opencl {
    // Public fields, accessible in benchmarks
    cl_platform_id platform{};
    cl_device_id device{};
    cl_context context{};
    cl_command_queue commandQueue{};

    // Constructors, destructor
    Opencl() : Opencl(QueueProperties::create()) {}
    Opencl(const QueueProperties &queueProperties) : Opencl(queueProperties, ContextProperties::create()) {}
    Opencl(const QueueProperties &queueProperties, const ContextProperties &contextProperties);
    ~Opencl();

    // Creating base OpenCL queue/context. Objects are saved inside the Opencl object
    // and released in its destructor
    cl_command_queue createQueue(QueueProperties queueProperties);
    cl_context createContext(const ContextProperties &contextProperties);

    // Getter methods for cl_device objects store inside the Opencl object. Single device version
    // will throw, if multiple devices are requested.
    cl_device_id getDevice(DeviceSelection deviceSelection);
    std::vector<cl_device_id> getDevices(DeviceSelection deviceSelection, bool requireSuccess);

    // Get helper used to query if certain extensions are supported by the OpenCL implementation
    const ExtensionsHelper &getExtensions();

  private:
    // Queriers subDevices of the root device and creates them if any. This method is only called when
    // it's necessary, i.e. user specified some subDevices in ContextProperties.
    bool createSubDevices(bool requireSuccess);

    // Internal fields managed by the Opencl class
    cl_device_id rootDevice;
    std::vector<cl_device_id> subDevices{};
    std::vector<cl_context> contexts{};
    std::vector<cl_command_queue> commandQueues{};
    std::unique_ptr<ExtensionsHelper> extensionsHelper{};
};

} // namespace OCL
using namespace OCL;
