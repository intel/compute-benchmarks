/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/intel_product/get_intel_product.h"
#include "framework/l0/levelzero.h"

IntelProduct getIntelProduct(ze_device_handle_t device);
IntelProduct getIntelProduct(const ze_device_properties_t &deviceProperties);
IntelProduct getIntelProduct(const LevelZero &levelzero);
