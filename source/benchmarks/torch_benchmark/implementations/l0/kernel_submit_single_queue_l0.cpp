/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "common.hpp"
#include "definitions/kernel_submit_single_queue.h"

#include <memory>
#include <unordered_map>

constexpr uint32_t alloc_size_double = 2;
constexpr uint32_t alloc_size_int = 3;

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
TestResult create_and_copy_host_arrays(LevelZero &l0,
                                       DataType data_type,
                                       double *p2_block,
                                       int *p3_block,
                                       uint32_t num_params,
                                       size_t length,
                                       std::vector<HostMemory<T>> &host_array) {
    host_array.reserve(num_params);
    for (uint32_t i = 0; i < num_params; ++i) {
        host_array.emplace_back(l0, length);
        if (data_type == DataType::CopyableObject)
            init_copyable_object_host(reinterpret_cast<CopyableObject *>(host_array[i].getPtr()), length, p2_block, p3_block, i);
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

        arg_storage.push_back(static_cast<void *>(res));
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

        // Now build kernel_arguments pointing to elements in arg_storage
        for (size_t i = 0; i < arg_storage.size(); i++) {
            kernel_arguments.push_back(&arg_storage[i]);
        }
    }
    return TestResult::Success;
}

static TestResult verify_result(ze_command_list_handle_t cmdList,
                                LevelZero &l0) {
    // setup
    constexpr uint32_t num_verify = 2;
    constexpr uint32_t length = 1;

    DeviceMemory<int> result_d(l0, length);
    DeviceMemory<int> device_array(l0, length * num_verify);
    std::vector<HostMemory<int>> host_array;
    ASSERT_TEST_RESULT_SUCCESS(create_and_copy_host_arrays<int>(l0, DataType::Int32,
                                                                nullptr, nullptr,
                                                                num_verify, length, host_array));
    // create kernel
    std::string kernel_file_name = "torch_benchmark_elementwise_sum_2.cl";
    std::string kernel_name = "elementwise_sum_2_int";
    Kernel kernel_verify{l0, kernel_file_name, kernel_name};
    constexpr ze_group_count_t dispatch{1u, 1u, 1u};
    constexpr ze_group_size_t group_sizes{1u, 1u, 1u};

    for (uint32_t i = 0; i < num_verify; i++) {
        for (uint32_t j = 0; j < length; ++j) {
            host_array[i].getPtr()[j] = 1;
        }

        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(
            cmdList,
            device_array.getPtr() + i * length,
            host_array[i].getPtr(),
            sizeof(int) * length,
            nullptr, 0, nullptr));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, UINT64_MAX));

    // submit kernel
    std::vector<void *> kernel_arguments;
    std::vector<void *> arg_storage;
    ASSERT_TEST_RESULT_SUCCESS(set_kernel_args("add", result_d.getPtr(), num_verify, device_array.getPtr(), length, kernel_arguments, arg_storage));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(cmdList, kernel_verify.get(), dispatch, group_sizes, kernel_arguments.data(),
                                                                          nullptr, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, UINT64_MAX));

    // do D2H after iterations to verify the result
    HostMemory<int> result_h(l0, length);
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, result_h.getPtr(), result_d.getPtr(), length * sizeof(int), nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, UINT64_MAX));

    // verify result
    if (*result_h.getPtr() != num_verify) {
        std::cerr << "Result verification failed: expected " << num_verify << " but got " << *result_h.getPtr() << std::endl;
        return TestResult::Error;
    }

    return TestResult::Success;
}

template <typename T>
static TestResult runBenchmark(const KernelSubmitSingleQueueArguments &args, ComboProfilerWithStats &profiler, Statistics &statistics) {
    // setup
    std::set<std::string> kernel_names;
    if (args.kernelName == KernelName::Empty) {
        kernel_names.insert("empty");
    } else if (args.kernelDataType == DataType::Mixed) {
        kernel_names.insert("add_mixed");
    } else if (args.kernelName == KernelName::Add) {
        kernel_names.insert("add");
    }

    uint32_t wgc = args.kernelWGCount;
    uint32_t wgs = args.kernelWGSize;
    size_t length = wgc * wgs;
    uint32_t num_params = args.kernelParamsNum;
    bool h2d = (args.kernelSubmitPattern == KernelSubmitPattern::H2d_before_batch);
    bool d2h = (args.kernelSubmitPattern == KernelSubmitPattern::D2h_after_batch);
    std::unordered_map<std::string, Kernel> kernels;

    LevelZero l0;
    CommandList cmd_list(l0.context, l0.device, zeDefaultGPUImmediateCommandQueueDesc);

    uint32_t arraysize_of_b = num_params;
    std::unique_ptr<HostMemory<T>> h_a;
    std::vector<HostMemory<T>> h_b;
    // These are used only in Mixed DataType scenarios
    uint32_t arraysize_of_c = 0;
    uint32_t arraysize_of_d = 0;
    std::vector<HostMemory<float>> h_c;
    std::vector<HostMemory<int>> h_d;

    std::unique_ptr<DeviceMemory<T>> d_a;
    std::unique_ptr<DeviceMemory<T>> device_array_db;
    std::unique_ptr<DeviceMemory<float>> device_array_dc;
    std::unique_ptr<DeviceMemory<int>> device_array_dd;

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

        d_a = std::make_unique<DeviceMemory<T>>(l0, length);
        device_array_db = std::make_unique<DeviceMemory<T>>(l0, length * arraysize_of_b);

        ASSERT_TEST_RESULT_SUCCESS(create_and_copy_host_arrays<T>(l0, args.kernelDataType,
                                                                  p2_block_db, p3_block_db,
                                                                  arraysize_of_b, length, h_b));

        if (args.kernelDataType == DataType::Mixed) {
            double *p2_block_dc = nullptr;
            int *p3_block_dc = nullptr;
            device_array_dc = std::make_unique<DeviceMemory<float>>(l0, length * arraysize_of_c);
            ASSERT_TEST_RESULT_SUCCESS(create_and_copy_host_arrays<float>(l0, args.kernelDataType,
                                                                          p2_block_dc, p3_block_dc,
                                                                          arraysize_of_c, length, h_c));

            double *p2_block_dd = nullptr;
            int *p3_block_dd = nullptr;
            device_array_dd = std::make_unique<DeviceMemory<int>>(l0, length * arraysize_of_d);
            ASSERT_TEST_RESULT_SUCCESS(create_and_copy_host_arrays<int>(l0, args.kernelDataType,
                                                                        p2_block_dd, p3_block_dd,
                                                                        arraysize_of_d, length, h_d));
        }

        if (h2d || d2h) {
            h_a = std::make_unique<HostMemory<T>>(l0, length);
        }
    }

    // create kernel
    for (const auto &name : kernel_names) {
        std::string kernelFileName;
        std::string kernelName;
        if (name == "empty") {
            kernelName = "elementwise_sum_0";
            kernelFileName = "torch_benchmark_" + kernelName + ".cl";
        } else if (name == "add_mixed") {
            kernelName = "elementwise_sum_mixed";
            kernelFileName = "torch_benchmark_" + kernelName + ".cl";
        } else if (name == "add") {
            std::string suffix;
            if (args.kernelDataType == DataType::CopyableObject) {
                suffix = "copyable_obj";
            } else {
                suffix = DataTypeHelper::toOpenclC(args.kernelDataType);
            }
            switch (num_params) {
            case 1: {
                kernelName = "elementwise_sum_1_" + suffix;
                kernelFileName = "torch_benchmark_elementwise_sum_1.cl";
                break;
            }
            case 5: {
                kernelName = "elementwise_sum_5_" + suffix;
                kernelFileName = "torch_benchmark_elementwise_sum_5.cl";
                break;
            }
            case 10: {
                kernelName = "elementwise_sum_10_" + suffix;
                kernelFileName = "torch_benchmark_elementwise_sum_10.cl";
                break;
            }
            }
        }
        kernels.emplace(std::piecewise_construct, std::forward_as_tuple(name), std::forward_as_tuple(l0, kernelFileName, kernelName));
    }

    ze_group_count_t dispatch{wgc, 1u, 1u};
    ze_group_size_t group_sizes{wgs, 1u, 1u};
    std::vector<void *> kernel_arguments;
    std::vector<void *> arg_storage;

    // simple kernel validation
    ASSERT_TEST_RESULT_SUCCESS(verify_result(cmd_list.get(), l0));

    // benchmark
    for (size_t i = 0; i < args.iterations; ++i) {
        if (h2d && args.kernelName != KernelName::Empty) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmd_list.get(), d_a->getPtr(), h_a->getPtr(), length * sizeof(T), nullptr, 0, nullptr));
        }
        for (const auto &[name, kernel] : kernels) {
            profiler.measureStart();
            if (args.kernelDataType == DataType::Mixed) {
                ASSERT_TEST_RESULT_SUCCESS(set_kernel_args(name, d_a->getPtr(),
                                                           arraysize_of_b, device_array_db->getPtr(),
                                                           length, kernel_arguments, arg_storage,
                                                           arraysize_of_c, device_array_dc->getPtr(),
                                                           arraysize_of_d, device_array_dd->getPtr()));
            } else if (args.kernelName != KernelName::Empty) {
                ASSERT_TEST_RESULT_SUCCESS(set_kernel_args(name, d_a->getPtr(), num_params, device_array_db->getPtr(), length, kernel_arguments, arg_storage));
            }
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(cmd_list.get(), kernel.get(), dispatch, group_sizes, kernel_arguments.data(), nullptr, nullptr, 0, nullptr));
            profiler.measureEnd();
            profiler.pushStats(statistics);
        }
        if (d2h && args.kernelName != KernelName::Empty) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmd_list.get(), h_a->getPtr(), d_a->getPtr(), length * sizeof(T), nullptr, 0, nullptr));
        }
        if (args.kernelBatchSize > 0 && ((i + 1) % args.kernelBatchSize == 0)) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmd_list.get(), UINT64_MAX));
        }
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmd_list.get(), UINT64_MAX));

    return TestResult::Success;
}

static TestResult run(const KernelSubmitSingleQueueArguments &args, Statistics &statistics) {
    ComboProfilerWithStats profiler(Configuration::get().profilerType);

    if (isNoopRun()) {
        profiler.pushNoop(statistics);
        return TestResult::Nooped;
    }

    // validate arguments
    if (args.kernelParamsNum != 1u && args.kernelParamsNum != 5u && args.kernelParamsNum != 10u) {
        std::cerr << "KernelParamsNum must be 1, 5, or 10" << std::endl;
        return TestResult::InvalidArgs;
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
