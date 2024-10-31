#include <sycl/sycl.hpp>

class Tensor4D;

#define random_float() (rand() / double(RAND_MAX) * 20. - 10.)

Tensor4D run_kernel_assign(const Tensor4D &inp, sycl::queue *q);
Tensor4D run_kernel_sin(const Tensor4D &inp, sycl::queue *q);
Tensor4D run_model(Tensor4D inp, sycl::queue *q, int op_multiplier);

// In IPEX/ITEX, device memory are allocated and reused, and released at last.
// here is a very simple mock for this behavior.

struct DeviceMemoryInfo {
    float *data; // float is enough for the demo
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
    sycl::queue *q;
    std::size_t allocCnt = 0, usedCnt = 0;
};

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