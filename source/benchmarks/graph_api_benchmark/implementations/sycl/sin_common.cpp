/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "sin_common.h"

Tensor4D run_kernel_assign(const Tensor4D &input, sycl::queue &Queue) {
    Tensor4D output(input.A, input.B, input.C, input.D);

    using vec4 = sycl::vec<float, 4>;

    vec4 *source = reinterpret_cast<vec4 *>(input.data);
    vec4 *dest = reinterpret_cast<vec4 *>(output.data);
    Queue.submit([&](sycl::handler &h) {
        h.parallel_for(output.count() / 4, [=](sycl::item<1> item) {
            int idx = item.get_id(0);
            dest[idx] = source[idx];
        });
    });

    return output;
}

Tensor4D run_kernel_sin(const Tensor4D &input, sycl::queue &Queue) {
    Tensor4D output(input.A, input.B, input.C, input.D);

    float *source = input.data;
    float *dest = output.data;

    Queue.submit([&](sycl::handler &h) {
        h.parallel_for(output.count(), [=](sycl::item<1> item) {
            int idx = item.get_id(0);
            dest[idx] = sycl::sin(source[idx]);
        });
    });
    return output;
}

Tensor4D run_model(Tensor4D input, sycl::queue &Queue, int kernelIterations) {

    Tensor4D output = run_kernel_assign(input, Queue);

    for (int itr = 0; itr < kernelIterations; ++itr) {
        input = output;
        output = run_kernel_sin(input, Queue);

        deviceMemMgr.free(input.data);
    }
    return output;
}

DeviceMemoryManager deviceMemMgr;

void DeviceMemoryManager::init(sycl::queue *queue) {
    assert(allocCnt == 0 && usedCnt == 0 && memInfos.size() == 0 && "memory leak");
    this->queue = queue;
}

void DeviceMemoryManager::deinit() {
    assert(usedCnt == 0 && "memory leak");

    for (; memInfos.size() > 0;) {
        DeviceMemoryInfo info = memInfos.back();
        assert(!info.used && "memory leak");
        sycl::free(info.data, *queue);
        memInfos.pop_back();
        allocCnt--;
    }
    assert(allocCnt == 0 && "memory leak");
}

float *DeviceMemoryManager::alloc(size_t count) {
    if (count == 0) {
        return nullptr;
    }

    for (auto &info : memInfos) {
        if (info.count >= count && !info.used) {
            usedCnt++;
            info.used = true;
            return info.data;
        }
    }

    usedCnt++;
    allocCnt++;
    float *deviceptr = sycl::malloc_device<float>(count, *queue);
    DeviceMemoryInfo info;
    info.data = deviceptr;
    info.count = count;
    info.used = true;
    memInfos.push_back(info);

    return deviceptr;
}

void DeviceMemoryManager::free(void *data) {
    for (auto &info : memInfos) {
        if (info.data == data) {
            assert(info.used && "double free");
            info.used = false;
            usedCnt--;
            return;
        }
    }
    assert(!"should not reach here");
}