/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/intel_product/get_intel_product.h"
#include "framework/ocl/opencl.h"

IntelProduct getIntelProduct(cl_device_id device);
IntelProduct getIntelProduct(const Opencl &opencl);
