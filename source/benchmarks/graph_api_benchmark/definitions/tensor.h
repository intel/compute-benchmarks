#pragma once

#include <sycl/sycl.hpp>

class DeviceMemoryManager;

extern DeviceMemoryManager g_devMemMgr;

class Tensor4D {
  public:
    Tensor4D(int A, int B, int C, int D) {
        this->A = A;
        this->B = B;
        this->C = C;
        this->D = D;
        this->data = g_devMemMgr.alloc(count());
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
