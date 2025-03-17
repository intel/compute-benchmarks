/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/mutate_graph.h"
#include "implementations/l0/memory_helper.h"

#include <gtest/gtest.h>
#include <level_zero/ze_api.h>
#include <list>

namespace {
struct TestEnv {
    ze_group_count_t groupCount{64u, 1u, 1u};
    const std::size_t size = 65536;
    std::shared_ptr<LevelZero> levelzero;
    ze_kernel_handle_t kernelSum = nullptr;
    ze_kernel_handle_t kernelMul = nullptr;
    ze_module_handle_t moduleSum;
    ze_module_handle_t moduleMul;
    ze_command_list_handle_t immCmdList;
    ze_command_queue_handle_t cmdQueue;
    std::mt19937 engine{0};
    std::uniform_real_distribution<float> distribution{-1.0, 1.0};
    mem_helper::DataFloatPtr inputData;
    mem_helper::DataFloatPtr refResult;
    mem_helper::DataFloatPtr outputData;
    mem_helper::DataFloatPtr resData;

    mem_helper::DataFloatPtr graphInputData;
    mem_helper::DataFloatPtr graphOutputData;
    ~TestEnv() {
        if (kernelSum == nullptr)
            return; // object was not initialized
        zeKernelDestroy(kernelSum);
        zeKernelDestroy(kernelMul);
        zeModuleDestroy(moduleSum);
        zeModuleDestroy(moduleMul);
        zeCommandListDestroy(immCmdList);
        zeCommandQueueDestroy(cmdQueue);
    }
};
TestResult create_command_list(TestEnv &env, ze_command_list_handle_t *graphCmdList) {

    ze_command_list_desc_t cmdListDesc = {ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC};

    cmdListDesc.commandQueueGroupOrdinal = env.levelzero->commandQueueDesc.ordinal;
    cmdListDesc.flags = ZE_COMMAND_LIST_FLAG_IN_ORDER;

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(
        env.levelzero->context, env.levelzero->device, &cmdListDesc, graphCmdList));
    return TestResult::Success;
}

void calcRefResults(std::size_t size,
                    const MutateGraphArguments &arguments,
                    float *inputData,
                    float *outputData,
                    float *refResult) {
    for (std::size_t i = 0; i < size; i++) {
        refResult[i] = outputData[i];
    }

    for (std::size_t i = 0; i < arguments.numKernels; i++) {

        if (i % arguments.changeRate == 0) {
            for (std::size_t i = 0; i < size; i++) {
                refResult[i] *= inputData[i];
            }
        } else {
            for (std::size_t i = 0; i < size; i++) {
                refResult[i] += inputData[i];
            }
        }
    }
}

bool checkResults(TestEnv &env, float *output_h, float *golden_h) {
    for (uint32_t idx = 0; idx < env.size; ++idx) {
        if ((fabs(output_h[idx] - golden_h[idx]) > 0.00001f)) {
            std::cout << "at (" << idx << "), expected " << golden_h[idx]
                      << ", but got " << output_h[idx] << std::endl;
            return false;
        }
    }
    return true;
}

void init_env(TestEnv &env) {

    env.levelzero = std::make_shared<LevelZero>();

    mem_helper::loadKernel(env.levelzero, "graph_api_benchmark_kernel_sum.spv", "kernel_sum", &env.kernelSum, &env.moduleSum);
    mem_helper::loadKernel(env.levelzero, "graph_api_benchmark_kernel_multiply.spv", "kernel_mul", &env.kernelMul, &env.moduleMul);

    uint32_t grpCnt[3] = {1, 1, 1};

    ZE_RESULT_SUCCESS_OR_ERROR(zeKernelSuggestGroupSize(env.kernelSum, env.size, 1, 1, grpCnt,
                                                        grpCnt + 1, grpCnt + 2));
    ZE_RESULT_SUCCESS_OR_ERROR(zeKernelSetGroupSize(env.kernelSum, grpCnt[0], grpCnt[1], grpCnt[2]));
    ZE_RESULT_SUCCESS_OR_ERROR(zeKernelSuggestGroupSize(env.kernelMul, env.size, 1, 1, grpCnt,
                                                        grpCnt + 1, grpCnt + 2));
    ZE_RESULT_SUCCESS_OR_ERROR(zeKernelSetGroupSize(env.kernelMul, grpCnt[0], grpCnt[1], grpCnt[2]));
    ze_command_queue_desc_t cmdQueueDesc = {ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC};
    cmdQueueDesc.ordinal = env.levelzero->commandQueueDesc.ordinal;
    cmdQueueDesc.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
    cmdQueueDesc.flags = ZE_COMMAND_QUEUE_FLAG_IN_ORDER;
    cmdQueueDesc.pNext = nullptr;

    ZE_RESULT_SUCCESS_OR_ERROR(
        zeCommandListCreateImmediate(env.levelzero->context, env.levelzero->device,
                                     &cmdQueueDesc, &env.immCmdList));

    // copy offload is only supported for immediate command lists
    cmdQueueDesc.pNext = nullptr;
    cmdQueueDesc.flags = 0;

    ZE_RESULT_SUCCESS_OR_ERROR(zeCommandQueueCreate(env.levelzero->context, env.levelzero->device,
                                                    &cmdQueueDesc, &env.cmdQueue));

    env.inputData = mem_helper::allocHost(env.levelzero, env.size);
    env.refResult = mem_helper::allocHost(env.levelzero, env.size);
    env.outputData = mem_helper::allocHost(env.levelzero, env.size);
    env.resData = mem_helper::allocHost(env.levelzero, env.size);

    env.graphInputData = mem_helper::allocDevice(env.levelzero, env.size);
    env.graphOutputData = mem_helper::allocDevice(env.levelzero, env.size);
}

TestResult create_mutable_list(TestEnv &env, ze_command_list_handle_t *graphCmdList) {

    ze_command_list_desc_t cmdListDesc = {ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC};
    ze_mutable_command_list_exp_desc_t mutCmdListDesc = {
        ZE_STRUCTURE_TYPE_MUTABLE_COMMAND_LIST_EXP_DESC,
        nullptr,
        0 // flags
    };
    cmdListDesc.pNext = &mutCmdListDesc;
    cmdListDesc.commandQueueGroupOrdinal = env.levelzero->commandQueueDesc.ordinal;
    cmdListDesc.flags = ZE_COMMAND_LIST_FLAG_IN_ORDER;

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(
        env.levelzero->context, env.levelzero->device, &cmdListDesc, graphCmdList));
    return TestResult::Success;
}
TestResult fill_mutable_list(const MutateGraphArguments &arguments,
                             TestEnv &env,
                             ze_command_list_handle_t cmdList,
                             std::vector<uint64_t> &identifiers) {
    float *dest = env.graphOutputData.get();
    float *source = env.graphInputData.get();

    ASSERT_ZE_RESULT_SUCCESS(
        zeKernelSetArgumentValue(env.kernelSum, 0, sizeof(float *), &dest));
    ASSERT_ZE_RESULT_SUCCESS(
        zeKernelSetArgumentValue(env.kernelSum, 1, sizeof(float *), &source));

    ze_kernel_handle_t kernels[2] = {env.kernelSum, env.kernelMul};
    for (uint32_t i = 0; i < arguments.numKernels; ++i) {

        if (i % arguments.changeRate == 0) {
            ze_mutable_command_exp_flags_t mutationFlags =
                ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_ARGUMENTS |
                ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_COUNT |
                ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_SIZE |
                ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_INSTRUCTION;
            ze_mutable_command_id_exp_desc_t cmdIdDesc = {
                ZE_STRUCTURE_TYPE_MUTABLE_COMMAND_ID_EXP_DESC, // stype
                nullptr,                                       // pNext
                mutationFlags};
            uint64_t mutableKernelCommandId = 0;
            zeCommandListGetNextCommandIdWithKernelsExp(cmdList, &cmdIdDesc, 2, kernels, &mutableKernelCommandId);
            identifiers.push_back(mutableKernelCommandId);
        }
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(
            cmdList, env.kernelSum, &env.groupCount, nullptr, 0, nullptr));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));
    return TestResult::Success;
}
TestResult mutate_list(TestEnv &env,
                       ze_command_list_handle_t cmdList,
                       std::vector<uint64_t> &identifiers) {
    uint32_t grpCnt[3] = {1, 1, 1};

    ASSERT_ZE_RESULT_SUCCESS(zeKernelSuggestGroupSize(env.kernelMul, env.size, 1, 1, grpCnt,
                                                      grpCnt + 1, grpCnt + 2));
    float *dest = env.graphOutputData.get();
    float *source = env.graphInputData.get();

    for (uint64_t identifier : identifiers) {

        zeCommandListUpdateMutableCommandKernelsExp(cmdList, 1, &identifier, &env.kernelMul);

        // modify group count
        ze_group_count_t groupCount = env.groupCount;
        ze_mutable_group_count_exp_desc_t groupCountDesc = {
            ZE_STRUCTURE_TYPE_MUTABLE_GROUP_COUNT_EXP_DESC, // stype
            nullptr,                                        // pNext
            identifier,                                     // commandId
            &groupCount                                     // pGroupCount
        };

        ze_mutable_group_size_exp_desc_t groupSizeDesc = {
            ZE_STRUCTURE_TYPE_MUTABLE_GROUP_SIZE_EXP_DESC, // stype
            &groupCountDesc,                               // pNext
            identifier,                                    // commandId
            grpCnt[0],                                     // groupSizeX
            grpCnt[1],                                     // groupSizeY
            grpCnt[2],                                     // groupSizeZ
        };

        ze_mutable_kernel_argument_exp_desc_t krnlArgMemoryDesc = {
            ZE_STRUCTURE_TYPE_MUTABLE_KERNEL_ARGUMENT_EXP_DESC, // stype
            &groupSizeDesc,                                     // pNext
            identifier,                                         // commandId
            0,                                                  // argIndex
            sizeof(void *),                                     // argSize
            &dest                                               // pArgValue
        };

        ze_mutable_kernel_argument_exp_desc_t krnlArgScalarDesc = {
            ZE_STRUCTURE_TYPE_MUTABLE_KERNEL_ARGUMENT_EXP_DESC, // stype
            &krnlArgMemoryDesc,                                 // pNext
            identifier,                                         // commandId
            1,                                                  // argIndex
            sizeof(void *),                                     // argSize
            &source                                             // pArgValue
        };

        // Prepare to update mutable commands
        ze_mutable_commands_exp_desc_t desc = {
            ZE_STRUCTURE_TYPE_MUTABLE_COMMANDS_EXP_DESC, // stype
            &krnlArgScalarDesc,                          // pNext
            0                                            // flags
        };
        zeCommandListUpdateMutableCommandsExp(cmdList, &desc);
    }
    zeCommandListClose(cmdList);
    return TestResult::Success;
}
TestResult fill_list(const MutateGraphArguments &arguments, TestEnv &env, ze_command_list_handle_t cmdList) {
    float *dest = env.graphOutputData.get();
    float *source = env.graphInputData.get();

    ASSERT_ZE_RESULT_SUCCESS(
        zeKernelSetArgumentValue(env.kernelSum, 0, sizeof(float *), &dest));
    ASSERT_ZE_RESULT_SUCCESS(
        zeKernelSetArgumentValue(env.kernelSum, 1, sizeof(float *), &source));

    ASSERT_ZE_RESULT_SUCCESS(
        zeKernelSetArgumentValue(env.kernelMul, 0, sizeof(float *), &dest));
    ASSERT_ZE_RESULT_SUCCESS(
        zeKernelSetArgumentValue(env.kernelMul, 1, sizeof(float *), &source));

    for (uint32_t i = 0; i < arguments.numKernels; ++i) {

        if (i % arguments.changeRate == 0) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(
                cmdList, env.kernelMul, &env.groupCount, nullptr, 0, nullptr));
        } else {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(
                cmdList, env.kernelSum, &env.groupCount, nullptr, 0, nullptr));
        }
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));
    return TestResult::Success;
}

TestResult fill_list_base(const MutateGraphArguments &arguments, TestEnv &env, ze_command_list_handle_t cmdList) {
    float *dest = env.graphOutputData.get();
    float *source = env.graphInputData.get();

    ASSERT_ZE_RESULT_SUCCESS(
        zeKernelSetArgumentValue(env.kernelSum, 0, sizeof(float *), &dest));
    ASSERT_ZE_RESULT_SUCCESS(
        zeKernelSetArgumentValue(env.kernelSum, 1, sizeof(float *), &source));

    for (uint32_t i = 0; i < arguments.numKernels; ++i) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(
            cmdList, env.kernelSum, &env.groupCount, nullptr, 0, nullptr));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));
    return TestResult::Success;
}

TestResult test_correctness(const MutateGraphArguments &arguments, TestEnv &env) {
    // reference results
    calcRefResults(env.size, arguments, env.inputData.get(), env.outputData.get(), env.refResult.get());

    // warm-up & results verification

    ASSERT_ZE_RESULT_SUCCESS(
        zeCommandListAppendMemoryCopy(env.immCmdList, env.graphInputData.get(), env.inputData.get(),
                                      env.size * sizeof(float), nullptr, 0, nullptr));

    ASSERT_ZE_RESULT_SUCCESS(
        zeCommandListAppendMemoryCopy(env.immCmdList, env.graphOutputData.get(), env.outputData.get(),
                                      env.size * sizeof(float), nullptr, 0, nullptr));

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(
        env.immCmdList, std::numeric_limits<uint64_t>::max()));
    ze_command_list_handle_t cmdList;
    if (arguments.canUpdate) {
        std::vector<uint64_t> identifiers;
        ASSERT_TEST_RESULT_SUCCESS(create_mutable_list(env, &cmdList));
        ASSERT_TEST_RESULT_SUCCESS(fill_mutable_list(arguments, env, cmdList, identifiers));
        ASSERT_TEST_RESULT_SUCCESS(mutate_list(env, cmdList, identifiers));
    } else {
        ASSERT_TEST_RESULT_SUCCESS(create_command_list(env, &cmdList));
        ASSERT_TEST_RESULT_SUCCESS(fill_list(arguments, env, cmdList));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListImmediateAppendCommandListsExp(
        env.immCmdList, 1, &cmdList, nullptr, 0, nullptr));

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(
        env.immCmdList, std::numeric_limits<uint64_t>::max()));
    ASSERT_ZE_RESULT_SUCCESS(
        zeCommandListAppendMemoryCopy(env.immCmdList, env.resData.get(), env.graphOutputData.get(),
                                      env.size * sizeof(float), nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(
        env.immCmdList, std::numeric_limits<uint64_t>::max()));

    zeCommandListDestroy(cmdList);
    // if results don't match, fail the benchmark
    if (!checkResults(env, env.resData.get(), env.refResult.get())) {
        std::cout << "Check FAILED" << std::endl;
        return TestResult::Error;
    }
    return TestResult::Success;
}
} // namespace

static TestResult run(const MutateGraphArguments &arguments, Statistics &statistics) {

    MeasurementFields typeSelector(MeasurementUnit::Microseconds,
                                   MeasurementType::Cpu);
    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(),
                                   typeSelector.getType());
        return TestResult::Nooped;
    }

    TestEnv env;
    init_env(env);
    Timer timer;
    for (uint32_t i = 0; i < env.size; ++i) {
        env.inputData.get()[i] = env.distribution(env.engine);
    }

    for (uint32_t i = 0; i < env.size; ++i) {
        env.outputData.get()[i] = env.distribution(env.engine);
    }

    ASSERT_TEST_RESULT_SUCCESS(test_correctness(arguments, env));
    for (uint32_t i = 0; i < arguments.iterations; ++i) {
        ze_command_list_handle_t cmdList;
        std::vector<uint64_t> identifiers;
        if (!arguments.compareCreation) {
            if (arguments.canUpdate) {
                ASSERT_TEST_RESULT_SUCCESS(create_mutable_list(env, &cmdList));
                ASSERT_TEST_RESULT_SUCCESS(fill_mutable_list(arguments, env, cmdList, identifiers));
            } else {
                ASSERT_TEST_RESULT_SUCCESS(create_command_list(env, &cmdList));
            }
        }

        timer.measureStart();
        if (arguments.compareCreation) {
            if (arguments.canUpdate) {
                ASSERT_TEST_RESULT_SUCCESS(create_mutable_list(env, &cmdList));
                ASSERT_TEST_RESULT_SUCCESS(fill_mutable_list(arguments, env, cmdList, identifiers));
            } else {
                ASSERT_TEST_RESULT_SUCCESS(create_command_list(env, &cmdList));
                ASSERT_TEST_RESULT_SUCCESS(fill_list_base(arguments, env, cmdList));
            }
        } else {
            if (arguments.canUpdate) {
                ASSERT_TEST_RESULT_SUCCESS(mutate_list(env, cmdList, identifiers));
            } else {
                ASSERT_TEST_RESULT_SUCCESS(fill_list(arguments, env, cmdList));
            }
        }
        timer.measureEnd();

        zeCommandListDestroy(cmdList);
        statistics.pushValue(timer.get(), typeSelector.getUnit(),
                             typeSelector.getType());
    }
    return TestResult::Success;
};

[[maybe_unused]] static RegisterTestCaseImplementation<MutateGraph> registerTestCase(run, Api::L0);
