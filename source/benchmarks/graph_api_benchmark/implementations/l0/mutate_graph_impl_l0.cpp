/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/random_distribution.h"
#include "framework/utility/timer.h"

#include "definitions/mutate_graph.h"
#include "implementations/l0/memory_helper.h"

#include <gtest/gtest.h>
#include <level_zero/ze_api.h>
#include <list>

namespace {
struct TestEnv {
    ze_group_count_t groupCount{64u, 1u, 1u};
    const uint32_t size = 65536;
    MeasurementFields typeSelector = MeasurementFields(MeasurementUnit::Microseconds,
                                                       MeasurementType::Cpu);
    std::shared_ptr<LevelZero> levelzero;
    ze_kernel_handle_t kernelSum = nullptr;
    ze_kernel_handle_t kernelMul = nullptr;
    ze_module_handle_t moduleSum;
    ze_module_handle_t moduleMul;
    ze_command_list_handle_t immCmdList;
    std::vector<ze_event_handle_t> zeEvents;
    ze_event_pool_handle_t zePool = nullptr;
    std::unique_ptr<RandomDistribution> distribution;
    mem_helper::DataFloatPtr inputData;
    mem_helper::DataFloatPtr refResult;
    mem_helper::DataFloatPtr outputData;
    mem_helper::DataFloatPtr resData;

    mem_helper::DataFloatPtr graphInputData;
    mem_helper::DataFloatPtr graphOutputData;
};
TestResult createCommandList(TestEnv &env, bool useInOrder, bool isMutable, ze_command_list_handle_t *graphCmdList) {

    ze_command_list_desc_t cmdListDesc = {ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC};

    ze_mutable_command_list_exp_desc_t mutCmdListDesc = {
        ZE_STRUCTURE_TYPE_MUTABLE_COMMAND_LIST_EXP_DESC,
        nullptr,
        0 // flags
    };
    if (isMutable) {
        cmdListDesc.pNext = &mutCmdListDesc;
    }
    cmdListDesc.commandQueueGroupOrdinal = env.levelzero->commandQueueDesc.ordinal;
    if (useInOrder) {
        cmdListDesc.flags = ZE_COMMAND_LIST_FLAG_IN_ORDER;
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(
        env.levelzero->context, env.levelzero->device, &cmdListDesc, graphCmdList));
    return TestResult::Success;
}

/*
    As an algorithm we use consecutive addition and multiplication.
    We start with addition only, and depending on arguments.changeRate,
    we change some addition operations to multiplication operations.
    (for example changeRate = 3 means that every third operation will be changed)
    Because both addition and multiplication are very quick, we assume that they take the similar amount of time,
    and because of that we can say that all graphs should take the same amount of time to execute.
*/
void calcRefResults(uint32_t size,
                    const MutateGraphArguments &arguments,
                    float *inputData,
                    float *outputData,
                    float *refResult) {
    for (uint32_t position = 0; position < size; position++) {
        refResult[position] = outputData[position];
    }

    for (uint32_t kernelId = 0; kernelId < arguments.numKernels; kernelId++) {

        if (kernelId % arguments.changeRate == 0) {
            for (uint32_t position = 0; position < size; position++) {
                refResult[position] *= inputData[position];
            }
        } else {
            for (uint32_t position = 0; position < size; position++) {
                refResult[position] += inputData[position];
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

TestResult initEnv(TestEnv &env, const MutateGraphArguments &arguments) {

    env.levelzero = std::make_shared<LevelZero>();

    L0::KernelHelper::loadKernel(*env.levelzero, "graph_api_benchmark_kernel_sum.spv", "kernel_sum", &env.kernelSum, &env.moduleSum);
    L0::KernelHelper::loadKernel(*env.levelzero, "graph_api_benchmark_kernel_multiply.spv", "kernel_mul", &env.kernelMul, &env.moduleMul);

    uint32_t grpCnt[3] = {1, 1, 1};

    ASSERT_ZE_RESULT_SUCCESS(zeKernelSuggestGroupSize(env.kernelSum, env.size, 1, 1, grpCnt,
                                                      grpCnt + 1, grpCnt + 2));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(env.kernelSum, grpCnt[0], grpCnt[1], grpCnt[2]));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSuggestGroupSize(env.kernelMul, env.size, 1, 1, grpCnt,
                                                      grpCnt + 1, grpCnt + 2));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(env.kernelMul, grpCnt[0], grpCnt[1], grpCnt[2]));
    ze_command_queue_desc_t cmdQueueDesc = {ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC};
    cmdQueueDesc.ordinal = env.levelzero->commandQueueDesc.ordinal;
    cmdQueueDesc.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
    if (arguments.useInOrder) {
        cmdQueueDesc.flags = ZE_COMMAND_QUEUE_FLAG_IN_ORDER;
    }
    cmdQueueDesc.pNext = nullptr;

    ASSERT_ZE_RESULT_SUCCESS(
        zeCommandListCreateImmediate(env.levelzero->context, env.levelzero->device,
                                     &cmdQueueDesc, &env.immCmdList));

    env.inputData = mem_helper::alloc(UsmMemoryPlacement::Host, env.levelzero, env.size);
    env.refResult = mem_helper::alloc(UsmMemoryPlacement::Host, env.levelzero, env.size);
    env.outputData = mem_helper::alloc(UsmMemoryPlacement::Host, env.levelzero, env.size);
    env.resData = mem_helper::alloc(UsmMemoryPlacement::Host, env.levelzero, env.size);

    env.graphInputData = mem_helper::alloc(UsmMemoryPlacement::Device, env.levelzero, env.size);
    env.graphOutputData = mem_helper::alloc(UsmMemoryPlacement::Device, env.levelzero, env.size);

    env.distribution = makeRandomDistribution(DistributionKind::Uniform, 0, 1);

    ze_event_desc_t edesc = {ZE_STRUCTURE_TYPE_EVENT_DESC};
    edesc.signal = 0;
    edesc.wait = 0;

    if (!arguments.useInOrder) {
        ze_event_pool_desc_t pdesc = {ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
        pdesc.count = static_cast<uint32_t>(arguments.numKernels - 1);
        pdesc.flags = 0;
        pdesc.pNext = nullptr;
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(env.levelzero->context, &pdesc, 1, &env.levelzero->device, &env.zePool));

        env.zeEvents.resize(arguments.numKernels - 1);
        for (uint32_t index = 0; index < arguments.numKernels - 1; index++) {
            edesc.index = index;
            ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(env.zePool, &edesc, &env.zeEvents[index]));
        }
    }
    return TestResult::Success;
}

TestResult destroyEnv(TestEnv &env) {
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(env.kernelSum));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(env.kernelMul));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(env.moduleSum));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(env.moduleMul));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(env.immCmdList));
    if (env.zePool != nullptr) {
        for (auto zeEvent : env.zeEvents) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(zeEvent));
        }
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(env.zePool));
    }
    return TestResult::Success;
}

TestResult resetEvents(TestEnv &env) {
    for (auto event : env.zeEvents) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(event));
    }
    return TestResult::Success;
}

TestResult fillMutableList(const MutateGraphArguments &arguments,
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
    for (uint32_t kernelId = 0; kernelId < arguments.numKernels; ++kernelId) {
        ze_event_handle_t previous = nullptr;
        ze_event_handle_t next = nullptr;
        if (!arguments.useInOrder) {
            if (kernelId > 0) {
                previous = env.zeEvents[kernelId - 1];
            }
            if (kernelId < arguments.numKernels - 1) {
                next = env.zeEvents[kernelId];
            }
        }
        if (kernelId % arguments.changeRate == 0) {
            ze_mutable_command_exp_flags_t mutationFlags =
                ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_ARGUMENTS |
                ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_COUNT |
                ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_SIZE;
            if (arguments.mutateIsa) {
                mutationFlags |= ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_INSTRUCTION;
            }
            ze_mutable_command_id_exp_desc_t cmdIdDesc = {
                ZE_STRUCTURE_TYPE_MUTABLE_COMMAND_ID_EXP_DESC, // stype
                nullptr,                                       // pNext
                mutationFlags};
            uint64_t mutableKernelCommandId = 0;
            uint32_t kernelGroupCount = 0;
            ze_kernel_handle_t *kernelGroupHandles = nullptr;
            if (arguments.mutateIsa) {
                kernelGroupCount = 2;
                kernelGroupHandles = kernels;
            }
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListGetNextCommandIdWithKernelsExp(cmdList, &cmdIdDesc, kernelGroupCount, kernelGroupHandles, &mutableKernelCommandId));
            identifiers.push_back(mutableKernelCommandId);
        }
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(
            cmdList, env.kernelSum, &env.groupCount, next, previous != nullptr ? 1 : 0, &previous));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));
    return TestResult::Success;
}
TestResult mutateList(TestEnv &env,
                      ze_command_list_handle_t cmdList,
                      std::vector<uint64_t> &identifiers,
                      uint32_t iteration,
                      bool mutateIsa) {
    uint32_t grpSize[3] = {1, 1, 1};

    auto currentKernelHandle = (iteration % 2 == 0) ? env.kernelMul : env.kernelSum;

    ASSERT_ZE_RESULT_SUCCESS(zeKernelSuggestGroupSize(currentKernelHandle, env.size, 1, 1, grpSize,
                                                      grpSize + 1, grpSize + 2));

    float *dest = env.graphOutputData.get();
    float *source = env.graphInputData.get();

    ze_group_count_t groupCount = env.groupCount;

    if (iteration % 2 != 0) {
        // preserve overall size of group size, change within dimensions, enough to cause group size mutation in odd iterations
        uint32_t tmpValue = grpSize[1];
        grpSize[1] = grpSize[0];
        grpSize[0] = tmpValue;

        // increase group count causes group count mutation in odd iterations
        groupCount.groupCountX += 1;

        // switch memory argument from default in odd iterations
        dest = env.graphInputData.get();
        source = env.graphOutputData.get();
    }

    for (uint64_t identifier : identifiers) {

        if (mutateIsa) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListUpdateMutableCommandKernelsExp(cmdList, 1, &identifier, &currentKernelHandle));
        }

        // modify group count
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
            grpSize[0],                                    // groupSizeX
            grpSize[1],                                    // groupSizeY
            grpSize[2],                                    // groupSizeZ
        };

        ze_mutable_kernel_argument_exp_desc_t krnlArgMemoryArg0Desc = {
            ZE_STRUCTURE_TYPE_MUTABLE_KERNEL_ARGUMENT_EXP_DESC, // stype
            &groupSizeDesc,                                     // pNext
            identifier,                                         // commandId
            0,                                                  // argIndex
            sizeof(void *),                                     // argSize
            &dest                                               // pArgValue
        };

        ze_mutable_kernel_argument_exp_desc_t krnlArgMemoryArg1Desc = {
            ZE_STRUCTURE_TYPE_MUTABLE_KERNEL_ARGUMENT_EXP_DESC, // stype
            &krnlArgMemoryArg0Desc,                             // pNext
            identifier,                                         // commandId
            1,                                                  // argIndex
            sizeof(void *),                                     // argSize
            &source                                             // pArgValue
        };

        // Prepare to update mutable commands
        ze_mutable_commands_exp_desc_t desc = {
            ZE_STRUCTURE_TYPE_MUTABLE_COMMANDS_EXP_DESC, // stype
            &krnlArgMemoryArg1Desc,                      // pNext
            0                                            // flags
        };
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListUpdateMutableCommandsExp(cmdList, &desc));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));
    return TestResult::Success;
}
TestResult fillList(const MutateGraphArguments &arguments, TestEnv &env, ze_command_list_handle_t cmdList, bool modified) {
    float *dest = env.graphOutputData.get();
    float *source = env.graphInputData.get();

    ASSERT_ZE_RESULT_SUCCESS(
        zeKernelSetArgumentValue(env.kernelSum, 0, sizeof(float *), &dest));
    ASSERT_ZE_RESULT_SUCCESS(
        zeKernelSetArgumentValue(env.kernelSum, 1, sizeof(float *), &source));
    if (modified) {
        ASSERT_ZE_RESULT_SUCCESS(
            zeKernelSetArgumentValue(env.kernelMul, 0, sizeof(float *), &dest));
        ASSERT_ZE_RESULT_SUCCESS(
            zeKernelSetArgumentValue(env.kernelMul, 1, sizeof(float *), &source));
    }
    for (uint32_t kernelId = 0; kernelId < arguments.numKernels; ++kernelId) {
        ze_event_handle_t previous = nullptr;
        ze_event_handle_t next = nullptr;
        if (!arguments.useInOrder) {
            if (kernelId > 0) {
                previous = env.zeEvents[kernelId - 1];
            }
            if (kernelId < arguments.numKernels - 1) {
                next = env.zeEvents[kernelId];
            }
        }
        if (modified && kernelId % arguments.changeRate == 0) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(
                cmdList, env.kernelMul, &env.groupCount, next, previous != nullptr ? 1 : 0, &previous));
        } else {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(
                cmdList, env.kernelSum, &env.groupCount, next, previous != nullptr ? 1 : 0, &previous));
        }
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));
    return TestResult::Success;
}

TestResult testCorrectness(const MutateGraphArguments &arguments, TestEnv &env) {
    MutateGraphArguments testCorrectnessArguments = arguments;
    // test correctness assumes kernel ISA mutation is ON
    testCorrectnessArguments.mutateIsa = true;

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
    ASSERT_TEST_RESULT_SUCCESS(createCommandList(env, arguments.useInOrder, arguments.canUpdate, &cmdList));
    if (arguments.canUpdate) {
        std::vector<uint64_t> identifiers;
        ASSERT_TEST_RESULT_SUCCESS(fillMutableList(testCorrectnessArguments, env, cmdList, identifiers));
        ASSERT_TEST_RESULT_SUCCESS(mutateList(env, cmdList, identifiers, 0, testCorrectnessArguments.mutateIsa));
    } else {
        ASSERT_TEST_RESULT_SUCCESS(fillList(arguments, env, cmdList, true));
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

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    // if results don't match, fail the benchmark
    if (!checkResults(env, env.resData.get(), env.refResult.get())) {
        std::cout << "Check FAILED" << std::endl;
        return TestResult::Error;
    }
    resetEvents(env);
    return TestResult::Success;
}

TestResult runCreate(const MutateGraphArguments &arguments, Statistics &statistics, TestEnv &env) {
    Timer timer;
    std::vector<uint64_t> identifiers;
    identifiers.reserve(arguments.numKernels);
    ze_command_list_handle_t cmdList = nullptr;
    for (uint32_t iteration = 0; iteration < arguments.iterations; ++iteration) {
        timer.measureStart();
        ASSERT_TEST_RESULT_SUCCESS(createCommandList(env, arguments.useInOrder, arguments.canUpdate, &cmdList));
        if (arguments.canUpdate) {
            ASSERT_TEST_RESULT_SUCCESS(fillMutableList(arguments, env, cmdList, identifiers));
        } else {
            ASSERT_TEST_RESULT_SUCCESS(fillList(arguments, env, cmdList, false));
        }
        timer.measureEnd();

        statistics.pushValue(timer.get(), env.typeSelector.getUnit(),
                             env.typeSelector.getType());
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
        identifiers.clear();
    }
    return TestResult::Success;
}

TestResult runInit(const MutateGraphArguments &arguments, Statistics &statistics, TestEnv &env) {
    Timer timer;
    std::vector<uint64_t> identifiers;
    identifiers.reserve(arguments.numKernels);
    ze_command_list_handle_t cmdList = nullptr;
    ASSERT_TEST_RESULT_SUCCESS(createCommandList(env, arguments.useInOrder, arguments.canUpdate, &cmdList));
    for (uint32_t iteration = 0; iteration < arguments.iterations; ++iteration) {
        timer.measureStart();
        if (arguments.canUpdate) {
            ASSERT_TEST_RESULT_SUCCESS(fillMutableList(arguments, env, cmdList, identifiers));
        } else {
            ASSERT_TEST_RESULT_SUCCESS(fillList(arguments, env, cmdList, false));
        }
        timer.measureEnd();

        statistics.pushValue(timer.get(), env.typeSelector.getUnit(),
                             env.typeSelector.getType());
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListReset(cmdList));
        identifiers.clear();
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    return TestResult::Success;
}

TestResult runMutate(const MutateGraphArguments &arguments, Statistics &statistics, TestEnv &env) {
    Timer timer;
    ze_command_list_handle_t cmdList = nullptr;
    std::vector<uint64_t> identifiers;
    identifiers.reserve(arguments.numKernels);
    ASSERT_TEST_RESULT_SUCCESS(createCommandList(env, arguments.useInOrder, arguments.canUpdate, &cmdList));
    if (arguments.canUpdate) {
        ASSERT_TEST_RESULT_SUCCESS(fillMutableList(arguments, env, cmdList, identifiers));
    } else {
        ASSERT_TEST_RESULT_SUCCESS(fillList(arguments, env, cmdList, true));
    }
    for (uint32_t iteration = 0; iteration < arguments.iterations; ++iteration) {
        timer.measureStart();
        if (arguments.canUpdate) {
            ASSERT_TEST_RESULT_SUCCESS(mutateList(env, cmdList, identifiers, iteration, arguments.mutateIsa));
        } else {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListReset(cmdList));
            ASSERT_TEST_RESULT_SUCCESS(fillList(arguments, env, cmdList, true));
        }
        timer.measureEnd();

        statistics.pushValue(timer.get(), env.typeSelector.getUnit(),
                             env.typeSelector.getType());
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    return TestResult::Success;
}

TestResult runExecute(const MutateGraphArguments &arguments, Statistics &statistics, TestEnv &env) {
    Timer timer;
    ze_command_list_handle_t cmdList = nullptr;
    std::vector<uint64_t> identifiers;
    identifiers.reserve(arguments.numKernels);
    ASSERT_TEST_RESULT_SUCCESS(createCommandList(env, arguments.useInOrder, arguments.canUpdate, &cmdList));
    if (arguments.canUpdate) {
        ASSERT_TEST_RESULT_SUCCESS(fillMutableList(arguments, env, cmdList, identifiers));
        ASSERT_TEST_RESULT_SUCCESS(mutateList(env, cmdList, identifiers, 0, arguments.mutateIsa));
    } else {
        ASSERT_TEST_RESULT_SUCCESS(fillList(arguments, env, cmdList, true));
    }

    for (uint32_t iteration = 0; iteration < arguments.iterations; ++iteration) {
        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListImmediateAppendCommandListsExp(
            env.immCmdList, 1, &cmdList, nullptr, 0, nullptr));

        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(
            env.immCmdList, std::numeric_limits<uint64_t>::max()));
        timer.measureEnd();
        resetEvents(env);
        statistics.pushValue(timer.get(), env.typeSelector.getUnit(),
                             env.typeSelector.getType());
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    return TestResult::Success;
}
} // namespace

static TestResult run(const MutateGraphArguments &arguments, Statistics &statistics) {

    TestEnv env;
    if (isNoopRun()) {
        statistics.pushUnitAndType(env.typeSelector.getUnit(),
                                   env.typeSelector.getType());
        return TestResult::Nooped;
    }

    ASSERT_TEST_RESULT_SUCCESS(initEnv(env, arguments));
    std::mt19937 gen{0};
    for (uint32_t position = 0; position < env.size; ++position) {
        env.inputData.get()[position] = static_cast<float>(env.distribution->get(gen)) / static_cast<float>(env.distribution->get(gen));
        env.outputData.get()[position] = static_cast<float>(env.distribution->get(gen)) / static_cast<float>(env.distribution->get(gen));
    }

    ASSERT_TEST_RESULT_SUCCESS(testCorrectness(arguments, env));
    switch (arguments.operationType) {
    case GraphOperationType::Init:
        ASSERT_TEST_RESULT_SUCCESS(runInit(arguments, statistics, env));
        break;
    case GraphOperationType::Mutate:
        ASSERT_TEST_RESULT_SUCCESS(runMutate(arguments, statistics, env));
        break;
    case GraphOperationType::Execute:
        ASSERT_TEST_RESULT_SUCCESS(runExecute(arguments, statistics, env));
        break;
    case GraphOperationType::Create:
        ASSERT_TEST_RESULT_SUCCESS(runCreate(arguments, statistics, env));
    default:
        break;
    }
    ASSERT_TEST_RESULT_SUCCESS(destroyEnv(env));
    return TestResult::Success;
};

[[maybe_unused]] static RegisterTestCaseImplementation<MutateGraph> registerTestCase(run, Api::L0);
