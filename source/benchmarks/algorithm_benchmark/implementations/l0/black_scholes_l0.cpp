/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/black_scholes.h"
#include "utility/workload_input.h"

#include <array>
#include <limits>
#include <type_traits>

using namespace L0;

template <typename ValueType>
static TestResult runWorkload(const BlackScholesArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    LevelZero levelzero{};
    if constexpr (std::is_same_v<ValueType, double>) {
        ze_device_module_properties_t moduleProperties{ZE_STRUCTURE_TYPE_DEVICE_MODULE_PROPERTIES};
        ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetModuleProperties(levelzero.device, &moduleProperties));
        if ((moduleProperties.flags & ZE_DEVICE_MODULE_FLAG_FP64) == 0) {
            return TestResult::DeviceNotCapable;
        }
    }

    const size_t bufferSize = BlackScholes::optionCount * sizeof(ValueType);
    const auto options = WorkloadInput::generateBlackScholesOptions<ValueType>(BlackScholes::optionCount);
    std::vector<ValueType> call(BlackScholes::optionCount);
    std::vector<ValueType> put(BlackScholes::optionCount);

    const char *buildFlags = std::is_same_v<ValueType, double> ? "-DDATA_TYPE_DOUBLE" : nullptr;
    ze_module_handle_t module{};
    ze_kernel_handle_t kernel{};
    if (auto result = L0::KernelHelper::loadKernel(levelzero, "algorithm_benchmark_black_scholes.cl", "black_scholes", &kernel, &module, buildFlags); result != TestResult::Success) {
        return result;
    }

    const ze_device_mem_alloc_desc_t deviceAllocationDesc{ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC};
    void *yearsBuffer{};
    void *strikeBuffer{};
    void *stockPriceBuffer{};
    void *callBuffer{};
    void *putBuffer{};
    const std::array<void **, 5> buffers = {&yearsBuffer, &strikeBuffer, &stockPriceBuffer, &callBuffer, &putBuffer};
    for (void **buffer : buffers) {
        ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &deviceAllocationDesc, bufferSize, 0, levelzero.device, buffer));
    }

    const ValueType riskFree = static_cast<ValueType>(BlackScholes::riskFree);
    const ValueType volatility = static_cast<ValueType>(BlackScholes::volatility);
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, BlackScholes::groupSize, 1u, 1u));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(riskFree), &riskFree));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 1, sizeof(volatility), &volatility));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 2, sizeof(yearsBuffer), &yearsBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 3, sizeof(strikeBuffer), &strikeBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 4, sizeof(stockPriceBuffer), &stockPriceBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 5, sizeof(callBuffer), &callBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 6, sizeof(putBuffer), &putBuffer));

    const ze_group_count_t groupCount{BlackScholes::optionCount / BlackScholes::groupSize, 1u, 1u};
    ze_command_queue_desc_t cmdListDesc = levelzero.commandQueueDesc;
    cmdListDesc.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
    cmdListDesc.flags |= ZE_COMMAND_QUEUE_FLAG_IN_ORDER;
    ze_command_list_handle_t cmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));

    Timer timer{};
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, yearsBuffer, options.years.data(), bufferSize, nullptr, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, strikeBuffer, options.strike.data(), bufferSize, nullptr, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, stockPriceBuffer, options.stockPrice.data(), bufferSize, nullptr, 0, nullptr));
        for (auto launch = 0u; launch < BlackScholes::kernelLaunchCount; launch++) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, nullptr, 0, nullptr));
        }
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, call.data(), callBuffer, bufferSize, nullptr, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, put.data(), putBuffer, bufferSize, nullptr, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, std::numeric_limits<uint64_t>::max()));
        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    for (void **buffer : buffers) {
        ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, *buffer));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    return TestResult::Success;
}

static TestResult run(const BlackScholesArguments &arguments, Statistics &statistics) {
    if (arguments.dataType == DataType::Float) {
        return runWorkload<float>(arguments, statistics);
    }
    if (arguments.dataType == DataType::Double) {
        return runWorkload<double>(arguments, statistics);
    }
    return TestResult::InvalidArgs;
}

static RegisterTestCaseImplementation<BlackScholes> registerTestCase(run, Api::L0);
