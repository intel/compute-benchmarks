/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/combo_profiler.h"
#include "framework/utility/file_helper.h"

#include "definitions/kernel_submit_single_queue.h"
#include "kernel_submit_common.hpp"

#include <gtest/gtest.h>
#include <level_zero/zer_api.h>
#include <unordered_map>

constexpr uint32_t alloc_size_double = 2;
constexpr uint32_t alloc_size_int = 3;

template <typename T>
TestResult l0_malloc_host(ze_context_handle_t context,
                          size_t length,
                          T **ptr) {
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocHost(context, &zeDefaultGPUHostMemAllocDesc, length * sizeof(T), alignof(T), (void **)ptr));
    return TestResult::Success;
}

static void init_copyable_object_host(CopyableObject *host_array, size_t length, double *p2_block, int *p3_block, int offset) {
    for (size_t j = 0; j < length; ++j) {
        host_array[j].v1 = static_cast<float>(1);
        host_array[j].p2 = p2_block + (offset * length + j) * alloc_size_double;
        host_array[j].p3 = p3_block + (offset * length + j) * alloc_size_int;
        host_array[j].size2_ = alloc_size_double;
        host_array[j].size3_ = alloc_size_int;
    }
}

template <typename T>
TestResult create_and_copy_host_arrays(ze_context_handle_t context,
                                       DataType data_type,
                                       double *p2_block,
                                       int *p3_block,
                                       uint32_t num_params,
                                       size_t length,
                                       T ***host_array) {
    ASSERT_TEST_RESULT_SUCCESS(l0_malloc_host<T *>(context, num_params, host_array));

    for (uint32_t i = 0; i < num_params; ++i) {
        ASSERT_TEST_RESULT_SUCCESS(l0_malloc_host<T>(context, length, &(*host_array)[i]));
        if (data_type == DataType::CopyableObject)
            init_copyable_object_host(reinterpret_cast<CopyableObject *>((*host_array)[i]), length, p2_block, p3_block, i);
    }
    return TestResult::Success;
}

template <typename DATATYPE1, typename DATATYPE2 = float, typename DATATYPE3 = int>
TestResult set_kernel_args(const std::string &kernel_name,
                           DATATYPE1 *res,
                           uint32_t num_params1, DATATYPE1 *device_array_1,
                           size_t total_elements,
                           std::vector<void *> &kernel_arguments,
                           std::vector<void *> &arg_storage,
                           uint32_t num_params2 = 0, DATATYPE2 *device_array_2 = nullptr,
                           uint32_t num_params3 = 0, DATATYPE3 *device_array_3 = nullptr) {
    if (kernel_name != "empty") {
        arg_storage.clear();
        kernel_arguments.clear();

        // Reserve space to prevent reallocation which would invalidate pointers
        size_t total_args = num_params1 + num_params2 + num_params3 + 1;
        arg_storage.reserve(total_args);
        kernel_arguments.reserve(total_args);

        for (uint32_t i = 0; i < num_params1; i++) {
            DATATYPE1 *ptr = device_array_1 + i * total_elements;
            arg_storage.push_back(static_cast<void *>(ptr));
        }
        if (num_params2 > 0 && device_array_2 != nullptr) {
            for (uint32_t i = 0; i < num_params2; i++) {
                DATATYPE2 *ptr = device_array_2 + i * total_elements;
                arg_storage.push_back(static_cast<void *>(ptr));
            }
        }
        if (num_params3 > 0 && device_array_3 != nullptr) {
            for (uint32_t i = 0; i < num_params3; i++) {
                DATATYPE3 *ptr = device_array_3 + i * total_elements;
                arg_storage.push_back(static_cast<void *>(ptr));
            }
        }
        arg_storage.push_back(static_cast<void *>(res));

        // Now build kernel_arguments pointing to elements in arg_storage
        for (size_t i = 0; i < arg_storage.size(); i++) {
            kernel_arguments.push_back(&arg_storage[i]);
        }
    }
    return TestResult::Success;
}

static TestResult verify_result(ze_command_list_handle_t cmdList,
                                LevelZero &l0) {
    // prepare parameters for verify kernel
    constexpr uint32_t num_verify = 2;
    uint32_t length = 1;
    ze_kernel_handle_t kernel_verify{};
    ze_module_handle_t module_verify{};
    ze_group_count_t dispatch{1u, 1u, 1u};
    ze_group_size_t group_sizes{1u, 1u, 1u};
    // use add op to verify
    // elementwise_kernel_2
    std::string kernel_name = "torch_benchmark_elementwise_sum_2";
    ASSERT_TEST_RESULT_SUCCESS(create_kernel(l0, kernel_name + ".cl", kernel_name + "_int", kernel_verify, module_verify));

    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel_verify, 1, 1, 1));
    int *result_d = nullptr;
    ASSERT_TEST_RESULT_SUCCESS(l0_malloc_device<int>(l0, length, result_d));
    int *device_array = nullptr;
    ASSERT_TEST_RESULT_SUCCESS(l0_malloc_device<int>(l0, length * num_verify, device_array));
    int **host_array = nullptr;
    ASSERT_TEST_RESULT_SUCCESS(create_and_copy_host_arrays<int>(l0.context, DataType::Int32,
                                                                nullptr, nullptr,
                                                                num_verify, length, &host_array));
    for (uint32_t i = 0; i < num_verify; i++) {
        for (uint32_t j = 0; j < length; ++j) {
            host_array[i][j] = static_cast<int>(1);
        }

        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(
            cmdList,
            device_array + i * length,
            host_array[i],
            sizeof(int) * length,
            nullptr, 0, nullptr));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, UINT64_MAX));
    std::vector<void *> kernel_arguments;
    std::vector<void *> arg_storage;
    ASSERT_TEST_RESULT_SUCCESS(set_kernel_args("add", result_d, num_verify, device_array, length, kernel_arguments, arg_storage));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(cmdList, kernel_verify, dispatch, group_sizes, kernel_arguments.data(),
                                                                          nullptr, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, UINT64_MAX));

    // do D2H after iterations to verify the result
    int *result_h = nullptr;
    ASSERT_TEST_RESULT_SUCCESS(l0_malloc_host<int>(l0.context, length, &result_h));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, result_h, result_d, length * sizeof(int), nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, UINT64_MAX));

    // verify result
    if (result_h[0] != static_cast<int>(num_verify)) {
        return TestResult::Error;
    }

    // cleanup
    for (uint32_t i = 0; i < num_verify; i++) {
        zeMemFree(l0.context, host_array[i]);
    }
    zeMemFree(l0.context, host_array);
    zeMemFree(l0.context, result_h);
    zeMemFree(l0.context, result_d);
    zeMemFree(l0.context, device_array);
    zeKernelDestroy(kernel_verify);

    return TestResult::Success;
}

template <typename T>
static TestResult runBenchmark(const KernelSubmitSingleQueueArguments &args, ComboProfilerWithStats &profiler, Statistics &statistics) {
    std::set<std::string> kernel_names;
    if (args.kernelName == KernelName::Empty) {
        kernel_names.insert("empty");
    } else if (args.kernelDataType == DataType::Mixed) {
        kernel_names.insert("add_mixed");
    } else if (args.kernelName == KernelName::Add) {
        kernel_names.insert("add");
    }

    // Get arguments
    size_t numIterations = args.iterations;
    size_t batch_size = args.kernelBatchSize;
    uint32_t wgc = args.kernelWGCount;
    uint32_t wgs = args.kernelWGSize;
    uint32_t num_params = args.kernelParamsNum;
    bool h2d = (args.kernelSubmitPattern == KernelSubmitPattern::H2d_before_batch);
    bool d2h = (args.kernelSubmitPattern == KernelSubmitPattern::D2h_after_batch);

    if (num_params < 1u) {
        std::cerr << "kernelParamsNum must be at least 1" << std::endl;
        return TestResult::InvalidArgs;
    } else if (num_params > 10u) {
        std::cerr << "kernelParamsNum must be at most 10" << std::endl;
        return TestResult::InvalidArgs;
    }

    LevelZero l0;
    std::unordered_map<std::string, ze_kernel_handle_t> kernels;
    size_t length = wgc * wgs;

    // Create an immediate command list
    ze_command_queue_desc_t qdesc{ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC};
    qdesc.ordinal = 0;
    qdesc.index = 0;
    qdesc.flags = ZE_COMMAND_QUEUE_FLAG_IN_ORDER;
    qdesc.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
    qdesc.priority = ZE_COMMAND_QUEUE_PRIORITY_NORMAL;
    ze_command_list_handle_t cmdListImmediate;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(l0.context, l0.device, &qdesc, &cmdListImmediate));

    // Prepare storage
    uint32_t arraysize_of_b = num_params;
    T *d_a = nullptr;
    T *h_a = nullptr;
    T **d_b = nullptr;
    T *device_array_db = nullptr;
    // These are used only in Mixed DataType scenarios
    uint32_t arraysize_of_c = 0;
    uint32_t arraysize_of_d = 0;
    float *device_array_dc = nullptr;
    int *device_array_dd = nullptr;
    float **d_c = nullptr;
    int **d_d = nullptr;

    if (args.kernelDataType == DataType::Mixed) {
        // The elementwise_sum_mixed kernel used in this scenario requires these fixed sizes
        // b/db suffixes are for pointers to double, c/dc for float, d/dd for int
        arraysize_of_b = 3;
        arraysize_of_c = 4;
        arraysize_of_d = 3;
    }

    if (args.kernelName != KernelName::Empty) {
        double *p2_block_db = nullptr;
        int *p3_block_db = nullptr;

        ASSERT_TEST_RESULT_SUCCESS(l0_malloc_device<T>(l0, length, d_a));
        ASSERT_TEST_RESULT_SUCCESS(l0_malloc_device<T>(l0, length * arraysize_of_b, device_array_db));

        ASSERT_TEST_RESULT_SUCCESS(create_and_copy_host_arrays<T>(l0.context, args.kernelDataType,
                                                                  p2_block_db, p3_block_db,
                                                                  arraysize_of_b, length, &d_b));

        if (args.kernelDataType == DataType::Mixed) {
            double *p2_block_dc = nullptr;
            int *p3_block_dc = nullptr;
            ASSERT_TEST_RESULT_SUCCESS(l0_malloc_device<float>(l0, length * arraysize_of_c, device_array_dc));
            ASSERT_TEST_RESULT_SUCCESS(create_and_copy_host_arrays<float>(l0.context, args.kernelDataType,
                                                                          p2_block_dc, p3_block_dc,
                                                                          arraysize_of_c, length, &d_c));

            double *p2_block_dd = nullptr;
            int *p3_block_dd = nullptr;
            ASSERT_TEST_RESULT_SUCCESS(l0_malloc_device<int>(l0, length * arraysize_of_d, device_array_dd));
            ASSERT_TEST_RESULT_SUCCESS(create_and_copy_host_arrays<int>(l0.context, args.kernelDataType,
                                                                        p2_block_dd, p3_block_dd,
                                                                        arraysize_of_d, length, &d_d));
        }

        if (h2d || d2h) {
            ASSERT_TEST_RESULT_SUCCESS(l0_malloc_host<T>(l0.context, length, &h_a));
        }
    }

    // Load kernels
    for (const auto &name : kernel_names) {
        ze_kernel_handle_t kernel{};
        ze_module_handle_t module{};
        if (name == "empty") {
            ASSERT_TEST_RESULT_SUCCESS(create_kernel(l0, "torch_benchmark_elementwise_sum_0.cl", "elementwise_sum_0", kernel, module));
        } else if (name == "add_mixed") {
            ASSERT_TEST_RESULT_SUCCESS(create_kernel(l0, "torch_benchmark_elementwise_sum_mixed.cl", "elementwise_sum_mixed", kernel, module));
        } else if (name == "add") {
            std::string suffix;
            if (args.kernelDataType == DataType::CopyableObject) {
                suffix = "copyable_obj";
            } else {
                suffix = DataTypeHelper::toOpenclC(args.kernelDataType);
            }
            switch (num_params) {
            case 1: {
                ASSERT_TEST_RESULT_SUCCESS(create_kernel(l0, "torch_benchmark_elementwise_sum_1.cl", "elementwise_sum_1_" + suffix, kernel, module));
                break;
            }
            case 5: {
                ASSERT_TEST_RESULT_SUCCESS(create_kernel(l0, "torch_benchmark_elementwise_sum_5.cl", "elementwise_sum_5_" + suffix, kernel, module));
                break;
            }
            case 10: {
                ASSERT_TEST_RESULT_SUCCESS(create_kernel(l0, "torch_benchmark_elementwise_sum_10.cl", "elementwise_sum_10_" + suffix, kernel, module));
                break;
            }
            default:
                std::cerr << "kernelParamsNum=" << args.kernelParamsNum << " is not the value used in test cases. 1, 5, 10 are supported." << std::endl;
                return TestResult::Error;
            }
        }
        kernels[name] = kernel;
    }

    ze_group_count_t dispatch{static_cast<uint32_t>(wgc), 1, 1};
    ze_group_size_t group_sizes{static_cast<uint32_t>(wgs), 1, 1};
    std::vector<void *> kernel_arguments;
    std::vector<void *> arg_storage;
    // simple kernel validation
    ASSERT_TEST_RESULT_SUCCESS(verify_result(cmdListImmediate, l0));

    // benchmarking
    for (size_t i = 0; i < numIterations; ++i) {
        if (h2d && args.kernelName != KernelName::Empty) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdListImmediate, d_a, h_a, length * sizeof(T), nullptr, 0, nullptr));
        }
        for (const auto &[name, kernel] : kernels) {
            profiler.measureStart();
            if (args.kernelDataType == DataType::Mixed) {
                ASSERT_TEST_RESULT_SUCCESS(set_kernel_args(name, d_a,
                                                           arraysize_of_b, device_array_db,
                                                           length, kernel_arguments, arg_storage,
                                                           arraysize_of_c, device_array_dc,
                                                           arraysize_of_d, device_array_dd));
            } else if (args.kernelName != KernelName::Empty) {
                ASSERT_TEST_RESULT_SUCCESS(set_kernel_args(name, d_a, num_params, device_array_db, length, kernel_arguments, arg_storage));
            }
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(cmdListImmediate, kernel, dispatch, group_sizes, kernel_arguments.data(), nullptr, nullptr, 0, nullptr));
            profiler.measureEnd();
            profiler.pushStats(statistics);
        }
        if (d2h && args.kernelName != KernelName::Empty) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdListImmediate, h_a, d_a, length * sizeof(T), nullptr, 0, nullptr));
        }
        if (batch_size > 0 && ((i + 1) % batch_size == 0)) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdListImmediate, UINT64_MAX));
        }
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdListImmediate, UINT64_MAX));

    // Cleanup
    for (auto &[name, kernel] : kernels) {
        if (kernel) {
            zeKernelDestroy(kernel);
        }
    }
    if (args.kernelName != KernelName::Empty) {
        if (h2d || d2h) {
            zeMemFree(l0.context, h_a);
        }
        for (uint32_t i = 0; i < arraysize_of_b; ++i) {
            zeMemFree(l0.context, d_b[i]);
        }
        zeMemFree(l0.context, d_b);
        zeMemFree(l0.context, device_array_db);
        zeMemFree(l0.context, d_a);
        if (args.kernelDataType == DataType::Mixed) {
            for (uint32_t i = 0; i < arraysize_of_c; ++i) {
                zeMemFree(l0.context, d_c[i]);
            }
            zeMemFree(l0.context, d_c);
            zeMemFree(l0.context, device_array_dc);
            for (uint32_t i = 0; i < arraysize_of_d; ++i) {
                zeMemFree(l0.context, d_d[i]);
            }
            zeMemFree(l0.context, d_d);
            zeMemFree(l0.context, device_array_dd);
        }
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdListImmediate));
    return TestResult::Success;
}

static TestResult run(const KernelSubmitSingleQueueArguments &args, Statistics &statistics) {
    ComboProfilerWithStats profiler(Configuration::get().profilerType);

    if (isNoopRun()) {
        std::cerr << "Noop run for Torch L0 benchmark" << std::endl;
        profiler.pushNoop(statistics);
        return TestResult::Nooped;
    }

    switch (args.kernelDataType) {
    case DataType::Int32:
        ASSERT_TEST_RESULT_SUCCESS(runBenchmark<int>(args, profiler, statistics));
        break;
    case DataType::Float:
        ASSERT_TEST_RESULT_SUCCESS(runBenchmark<float>(args, profiler, statistics));
        break;
    case DataType::Double:
    case DataType::Mixed:
        ASSERT_TEST_RESULT_SUCCESS(runBenchmark<double>(args, profiler, statistics));
        break;
    case DataType::CopyableObject:
        ASSERT_TEST_RESULT_SUCCESS(runBenchmark<CopyableObject>(args, profiler, statistics));
        break;
    default:
        return TestResult::InvalidArgs;
    }

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<KernelSubmitSingleQueue> registerTestCase(run, Api::L0);
