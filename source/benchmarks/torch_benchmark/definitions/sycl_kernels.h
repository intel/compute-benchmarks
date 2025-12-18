/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef MY_KERNEL_H
#define MY_KERNEL_H

#include <sycl/queue.hpp>

#ifndef EVENTLESS_SUBMIT
#define EVENTLESS_SUBMIT 0
#endif

inline void submit_kernel_empty(unsigned int wgc, unsigned int wgs, sycl::queue &q) {
#if EVENTLESS_SUBMIT
    sycl::ext::oneapi::experimental::nd_launch(q, sycl::nd_range<1>{wgc * wgs, wgs}, [=](sycl::nd_item<1> item) {
        ; // empty kernel
    });
#else
    q.parallel_for(sycl::nd_range<1>{wgc * wgs, wgs}, [=](sycl::nd_item<1>) {
        ; // empty kernel
    });
#endif
}

template <typename data_type, typename... Args>
void add(sycl::nd_item<1> item, data_type *sum, Args *...srcs) {
    size_t id = item.get_global_id(0);
    sum[id] = (srcs[id] + ...);
}

template <typename data_type, typename... Args>
void submit_kernel_add(unsigned int wgc, unsigned int wgs, sycl::queue &q, data_type *sum, Args *...srcs) {
#if EVENTLESS_SUBMIT
    sycl::ext::oneapi::experimental::nd_launch(q, sycl::nd_range<1>{wgc * wgs, wgs}, [=](sycl::nd_item<1> item) {
        add<data_type>(item, sum, srcs...);
    });
#else
    q.parallel_for(sycl::nd_range<1>{wgc * wgs, wgs}, [=](sycl::nd_item<1> item) {
        add<data_type>(item, sum, srcs...);
    });
#endif
}

template <typename data_type, typename... Args>
void submit_kernel_add(unsigned int wgc, unsigned int wgs, sycl::queue &q, sycl::event &depEvent, data_type *sum, Args *...srcs) {
#if EVENTLESS_SUBMIT
    sycl::ext::oneapi::experimental::submit_with_event(q, [&](sycl::handler &h) {
        h.depends_on(depEvent);
        sycl::ext::oneapi::experimental::nd_launch(h, sycl::nd_range<1>{wgc * wgs, wgs}, [=](sycl::nd_item<1> item) {
            add<data_type>(item, sum, srcs...);
        });
    });
#else
    q.parallel_for(sycl::nd_range<1>{wgc * wgs, wgs}, depEvent, [=](sycl::nd_item<1> item) {
        add<data_type>(item, sum, srcs...);
    });
#endif
}

template <typename data_type, typename... Args>
sycl::event submit_with_event_kernel_add(unsigned int wgc, unsigned int wgs, sycl::queue &q, data_type *sum, Args *...srcs) {
#if EVENTLESS_SUBMIT
    return sycl::ext::oneapi::experimental::submit_with_event(q, [&](sycl::handler &h) {
        sycl::ext::oneapi::experimental::nd_launch(h, sycl::nd_range<1>{wgc * wgs, wgs}, [=](sycl::nd_item<1> item) {
            add<data_type>(item, sum, srcs...);
        });
    });
#else
    return q.parallel_for(sycl::nd_range<1>{wgc * wgs, wgs}, [=](sycl::nd_item<1> item) {
        add<data_type>(item, sum, srcs...);
    });
#endif
}

template <typename data_type>
void submit_kernel_add(unsigned int wgc, unsigned int wgs, sycl::queue &q, data_type *sum, data_type **src, unsigned int add_num_params) {
    switch (add_num_params) {
    case 1:
        submit_kernel_add<data_type>(wgc, wgs, q, sum, src[0]);
        break;
    case 2:
        submit_kernel_add<data_type>(wgc, wgs, q, sum, src[0], src[1]);
        break;
    case 3:
        submit_kernel_add<data_type>(wgc, wgs, q, sum, src[0], src[1], src[2]);
        break;
    case 4:
        submit_kernel_add<data_type>(wgc, wgs, q, sum, src[0], src[1], src[2], src[3]);
        break;
    case 5:
        submit_kernel_add<data_type>(wgc, wgs, q, sum, src[0], src[1], src[2], src[3], src[4]);
        break;
    case 6:
        submit_kernel_add<data_type>(wgc, wgs, q, sum, src[0], src[1], src[2], src[3], src[4], src[5]);
        break;
    case 7:
        submit_kernel_add<data_type>(wgc, wgs, q, sum, src[0], src[1], src[2], src[3], src[4], src[5], src[6]);
        break;
    case 8:
        submit_kernel_add<data_type>(wgc, wgs, q, sum, src[0], src[1], src[2], src[3], src[4], src[5], src[6], src[7]);
        break;
    case 9:
        submit_kernel_add<data_type>(wgc, wgs, q, sum, src[0], src[1], src[2], src[3], src[4], src[5], src[6], src[7], src[8]);
        break;
    case 10:
        submit_kernel_add<data_type>(wgc, wgs, q, sum, src[0], src[1], src[2], src[3], src[4], src[5], src[6], src[7], src[8], src[9]);
        break;
    }
}

template <typename data_type1, typename data_type2, typename data_type3>
void submit_kernel_add_mixed_type(unsigned int wgc, unsigned int wgs, sycl::queue &q, data_type1 *sum, data_type1 *src0, data_type1 *src1, data_type1 *src2, data_type2 *src3, data_type2 *src4, data_type2 *src5, data_type2 *src6, data_type3 *src7, data_type3 *src8, data_type3 *src9) {
#if EVENTLESS_SUBMIT
    sycl::ext::oneapi::experimental::nd_launch(q, sycl::nd_range<1>{wgc * wgs, wgs}, [=](sycl::nd_item<1> item) {
        size_t id = item.get_global_id(0);
        sum[id] = (src0[id] + src1[id] + src2[id] + src3[id] + src4[id] + src5[id] + src6[id] + src7[id] + src8[id] + src9[id]);
    });
#else
    q.parallel_for(sycl::nd_range<1>{wgc * wgs, wgs}, [=](sycl::nd_item<1> item) {
        size_t id = item.get_global_id(0);
        sum[id] = (src0[id] + src1[id] + src2[id] + src3[id] + src4[id] + src5[id] + src6[id] + src7[id] + src8[id] + src9[id]);
    });
#endif
}

template <typename data_type>
void mul(sycl::nd_item<1> item, data_type *result, data_type *src1, data_type *src2) {
    size_t id = item.get_global_id(0);
    result[id] = src1[id] * src2[id];
}

template <typename data_type>
void submit_kernel_mul(unsigned int wgc, unsigned int wgs, sycl::queue &q, data_type *result, data_type *src1, data_type *src2) {
#if EVENTLESS_SUBMIT
    sycl::ext::oneapi::experimental::nd_launch(q, sycl::nd_range<1>{wgc * wgs, wgs}, [=](sycl::nd_item<1> item) {
        mul<data_type>(item, result, src1, src2);
    });
#else
    q.parallel_for(sycl::nd_range<1>{wgc * wgs, wgs}, [=](sycl::nd_item<1> item) {
        mul<data_type>(item, result, src1, src2);
    });
#endif
}

template <typename data_type>
void submit_kernel_write(sycl::queue &q, data_type *p1, int offset1, data_type *p2, int offset2) {
#if EVENTLESS_SUBMIT
    sycl::ext::oneapi::experimental::single_task(q, [=]() {
        *(p1 + offset1) = offset1;
        *(p2 + offset2) = offset2;
    });
#else
    q.single_task([=]() {
        *(p1 + offset1) = offset1;
        *(p2 + offset2) = offset2;
    });
#endif
}
#endif
