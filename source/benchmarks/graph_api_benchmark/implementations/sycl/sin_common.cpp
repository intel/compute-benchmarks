#include "sin_common.h"

#include <sycl/sycl.hpp>

Tensor4D run_kernel_assign(const Tensor4D &inp, sycl::queue *q) {
    Tensor4D outp(inp.A, inp.B, inp.C, inp.D);

    using vec4 = sycl::vec<float, 4>;

    vec4 *vsrc = reinterpret_cast<vec4 *>(inp.data);
    vec4 *vdst = reinterpret_cast<vec4 *>(outp.data);
    q->submit([&](sycl::handler &h) {
        h.parallel_for(outp.count() / 4, [=](sycl::item<1> item) {
            int idx = item.get_id(0);
            vdst[idx] = vsrc[idx];
        });
    });

    return outp;
}

Tensor4D run_kernel_sin(const Tensor4D &inp, sycl::queue *q) {
    Tensor4D outp(inp.A, inp.B, inp.C, inp.D);

    float *src = inp.data;
    float *dst = outp.data;

    q->submit([&](sycl::handler &h) {
        h.parallel_for(outp.count(), [=](sycl::item<1> item) {
            int idx = item.get_id(0);
            dst[idx] = sycl::sin(src[idx]);
        });
    });
    return outp;
}

Tensor4D run_model(Tensor4D inp, sycl::queue *q, int op_multiplier) {
    if (op_multiplier < 1) {
        op_multiplier = 1;
    }

    Tensor4D outp = run_kernel_assign(inp, q);

    for (int i = 0; i < op_multiplier; ++i) {
        inp = outp;
        outp = run_kernel_sin(inp, q);

        g_devMemMgr.free(inp.data);
    }
    return outp;
}

DeviceMemoryManager g_devMemMgr;

void DeviceMemoryManager::init(sycl::queue *q) {
    // std::cout << "DeviceMemoryManager::init, allocCnt" << allocCnt << " usedCnt " << usedCnt << " memInfos.size() " << memInfos.size() << std::endl;
    assert(allocCnt == 0 && usedCnt == 0 && memInfos.size() == 0 && "memory leak");
    this->q = q;
}

void DeviceMemoryManager::deinit() {
    // std::cout << "DeviceMemoryManager::deinit, allocCnt" << allocCnt << " usedCnt " << usedCnt << " memInfos.size() " << memInfos.size() << std::endl;
    assert(usedCnt == 0 && "memory leak");
    // for (auto &info : memInfos) {

    for (; memInfos.size() > 0;) {
        DeviceMemoryInfo info = memInfos.back();
        assert(!info.used && "memory leak");
        sycl::free(info.data, *q);
        memInfos.pop_back();
        allocCnt--;
        // std::cout << "->allocCnt " << allocCnt << " memInfos.size() " << memInfos.size() << std::endl;
    }
    assert(allocCnt == 0 && "memory leak");
    // std::cout << "DONE" << std::endl;
}

float *DeviceMemoryManager::alloc(size_t count) {
    if (count == 0) {
        return nullptr;
    }

    for (auto &info : memInfos) {
        if (info.count >= count && !info.used) {
            usedCnt++;
            info.used = true;
            // std::cout << "DeviceMemoryManager::alloc, allocCnt " << allocCnt << " usedCnt " << usedCnt << " memInfos.size() " << memInfos.size() << std::endl;
            return info.data;
        }
    }

    usedCnt++;
    allocCnt++;
    float *p = sycl::malloc_device<float>(count, *q);
    DeviceMemoryInfo info;
    info.data = p;
    info.count = count;
    info.used = true;
    memInfos.push_back(info);
    // std::cout << "DeviceMemoryManager::alloc, allocCnt " << allocCnt << " usedCnt " << usedCnt << " memInfos.size() " << memInfos.size() << std::endl;

    return p;
}

void DeviceMemoryManager::free(void *data) {
    // std::cout << "DeviceMemoryManager::free, allocCnt " << allocCnt << std::flush;
    for (auto &info : memInfos) {
        if (info.data == data) {
            assert(info.used && "double free");
            info.used = false;
            usedCnt--;
            // std::cout << " usedCnt " << usedCnt << " memInfos.size() " << memInfos.size() << std::endl;
            return;
        }
    }
    assert(!"should not reach here");
}