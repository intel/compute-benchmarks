/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <sycl/sycl.hpp>

class Tensor4D;

#define random_float() (rand() / double(RAND_MAX) * 20. - 10.)

Tensor4D run_kernel_assign(const Tensor4D &inp, sycl::queue &Queue);
Tensor4D run_kernel_sin(const Tensor4D &inp, sycl::queue &Queue);
Tensor4D run_model(Tensor4D inp, sycl::queue &Queue, int kernelIterations);

struct DeviceMemoryInfo {
    float *data; // float is enough for the benchmark
    size_t count;
    bool used;
};

class DeviceMemoryManager {
  public:
    DeviceMemoryManager() {}
    ~DeviceMemoryManager() {}
    void init(sycl::queue *q);
    void deinit();
    float *alloc(size_t count);
    void free(void *data);

  private:
    std::vector<DeviceMemoryInfo> memInfos;
    sycl::queue *queue;
    std::size_t allocCnt = 0, usedCnt = 0;
};

extern DeviceMemoryManager deviceMemMgr;

class Tensor4D {
  public:
    Tensor4D(int A, int B, int C, int D) {
        this->A = A;
        this->B = B;
        this->C = C;
        this->D = D;
        this->data = deviceMemMgr.alloc(count());
    }

    std::size_t count() const {
        return A * B * C * D;
    }

    int A;
    int B;
    int C;
    int D;
    float *data;
};