/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"
#include "framework/vk/shader_compiler.h"
#include "framework/vk/vulkan.h"

#include "definitions/multi_kernel_execution.h"

#include <gtest/gtest.h>
#include <vector>

using namespace VK;

static TestResult run(const MultiKernelExecutionArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (arguments.inOrderOverOOO || arguments.useMCL) {
        return TestResult::ApiNotCapable;
    }

    if (arguments.profiling) {
        return TestResult::NoImplementation;
    }

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    Vulkan vulkan;
    Timer timer;

    if (!vulkan.supportsDispatch1D(arguments.workgroupSize, arguments.workgroupCount)) {
        return TestResult::DeviceNotCapable;
    }

    std::vector<uint32_t> spirv;
    if (const TestResult result = ShaderCompiler::compileComputeShaderToSpirv("ulls_benchmark_empty_kernel.comp", {}, spirv);
        result != TestResult::Success) {
        return result;
    }
    const VulkanShaderModule shaderModule(vulkan, spirv);
    const VulkanComputePipeline pipeline(vulkan, shaderModule, static_cast<uint32_t>(arguments.workgroupSize), VK_NULL_HANDLE);

    const VkCommandBuffer commandBuffer = vulkan.allocateCommandBuffer();
    const VkCommandBufferBeginInfo commandBufferBeginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    ASSERT_VK_SUCCESS(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    for (auto kernelIndex = 0u; kernelIndex < arguments.kernelCount; kernelIndex++) {
        if (kernelIndex > 0) {
            recordInOrderExecutionBarrier(commandBuffer);
        }
        vkCmdDispatch(commandBuffer, static_cast<uint32_t>(arguments.workgroupCount), 1u, 1u);
    }
    ASSERT_VK_SUCCESS(vkEndCommandBuffer(commandBuffer));

    const VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
    };

    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        ASSERT_VK_SUCCESS(vkQueueSubmit(vulkan.queue, 1, &submitInfo, VK_NULL_HANDLE));
        ASSERT_VK_SUCCESS(vkQueueWaitIdle(vulkan.queue));
        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<MultiKernelExecution> registerTestCase(run, Api::Vulkan);
