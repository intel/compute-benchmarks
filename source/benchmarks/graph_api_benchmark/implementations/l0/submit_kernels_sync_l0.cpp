#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/submit_kernels_sync.h"

#include <iostream>

static TestResult run(const SubmitKernelsSyncArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    std::size_t numK = arguments.numKernels;
    Timer timer;

    // Initialize Level Zero
    LevelZero levelzero;

    // Allocate memory on the device
    const int size = 128;
    const size_t bytes = size * sizeof(int);

    const ze_group_count_t groupCount{size, 1, 1};

    ze_module_handle_t module;
    ze_kernel_handle_t kernel;

    ze_device_mem_alloc_desc_t allocDesc = {};
    allocDesc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;

    void *d_array;
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &allocDesc, bytes, 4u, levelzero.device, &d_array));

    int *h_array = new int[size];

    auto spirvModule = FileHelper::loadBinaryFile("graph_api_benchmark_increment.spv");
    if (spirvModule.size() == 0) {
        std::cerr << "Kernel not loaded\n";
        return TestResult::KernelNotFound;
    }

    ze_kernel_desc_t kernelDesc = {ZE_STRUCTURE_TYPE_KERNEL_DESC, nullptr, 0, "increment"};

    for (auto i = 0u; i < arguments.iterations; i++) {

        // Create events
        ze_event_pool_handle_t eventPool{};
        ze_event_handle_t event{};
        ze_event_handle_t *pEvents = new ze_event_handle_t[numK];

        ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
        eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP | ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
        eventPoolDesc.count = numK + 6;
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.device, &eventPool));

        ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
        eventDesc.index = 0;
        eventDesc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
        eventDesc.wait = ZE_EVENT_SCOPE_FLAG_HOST;

        ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &event));

        // Create an immediate command list
        ze_command_queue_desc_t commandQueueDesc{ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC};
        commandQueueDesc.mode = ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS;
        ze_command_list_desc_t cmdListDesc{};
        cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
        ze_command_list_handle_t cmdList{};
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));

        ze_module_desc_t moduleDesc{ZE_STRUCTURE_TYPE_MODULE_DESC};
        moduleDesc.format = ZE_MODULE_FORMAT_IL_SPIRV;
        moduleDesc.pInputModule = reinterpret_cast<const uint8_t *>(spirvModule.data());
        moduleDesc.inputSize = spirvModule.size();

        // Create module/kernel
        ASSERT_ZE_RESULT_SUCCESS(zeModuleCreate(levelzero.context, levelzero.device, &moduleDesc, &module, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &kernel));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, size, 1, 1));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(d_array), &d_array));

        // Prepare data
        for (size_t i = 0; i < size; ++i) {
            h_array[i] = static_cast<int>(i);
        }

        // Copy data to device
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, d_array, h_array, bytes, event, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListReset(cmdList));

        // warm-up
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, event, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListReset(cmdList));

        // Copy data to device
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, d_array, h_array, bytes, event, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListReset(cmdList));

        timer.measureStart();
        // std::cout << "track\n";

        // Append the kernel multiple times & execute
        for (std::size_t idx = 0; idx < numK; ++idx) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, event /* pEvents[idx] */, 0, nullptr));
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));
            ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
            ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListReset(cmdList));
        }
        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());

        // Copy results back to host
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, h_array, d_array, bytes, event, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListReset(cmdList));

        // Check results
        for (size_t i = 0; i < size; ++i) {
            if (h_array[i] != static_cast<int>(i + numK)) {
                return TestResult::Error;
            }
        }

        // Cleanup
        delete[] pEvents;
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, d_array));
    delete[] h_array;
    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<SubmitKernelsSync> registerTestCase(run, Api::L0);