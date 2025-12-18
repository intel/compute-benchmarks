/*
 * Copyright (C) 2024-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/error.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/random_distribution.h"
#include "framework/utility/timer.h"

#include "definitions/record_graph.h"

#include <gtest/gtest.h>
#include <level_zero/driver_experimental/public/zex_graph.h>
#include <level_zero/ze_api.h>
#include <list>
#include <stack>
#include <unordered_map>

namespace {

using zeGraphCreateExpFP = ze_result_t(ZE_APICALL *)(ze_context_handle_t context, ze_graph_handle_t *phGraph, void *pNext);
using zeCommandListBeginGraphCaptureExpFP = ze_result_t(ZE_APICALL *)(ze_command_list_handle_t hCommandList, void *pNext);
using zeCommandListBeginCaptureIntoGraphExpFP = ze_result_t(ZE_APICALL *)(ze_command_list_handle_t hCommandList, ze_graph_handle_t hGraph, void *pNext);
using zeCommandListEndGraphCaptureExpFP = ze_result_t(ZE_APICALL *)(ze_command_list_handle_t hCommandList, ze_graph_handle_t *phGraph, void *pNext);
using zeCommandListInstantiateGraphExpFP = ze_result_t(ZE_APICALL *)(ze_graph_handle_t hGraph, ze_executable_graph_handle_t *phGraph, void *pNext);
using zeCommandListAppendGraphExpFP = ze_result_t(ZE_APICALL *)(ze_command_list_handle_t hCommandList, ze_executable_graph_handle_t hGraph, void *pNext,
                                                                ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t *phWaitEvents);
using zeGraphDestroyExpFP = ze_result_t(ZE_APICALL *)(ze_graph_handle_t hGraph);
using zeExecutableGraphDestroyExpFP = ze_result_t(ZE_APICALL *)(ze_executable_graph_handle_t hGraph);

using zeCommandListIsGraphCaptureEnabledExpFP = ze_result_t(ZE_APICALL *)(ze_command_list_handle_t hCommandList);
using zeGraphIsEmptyExpFP = ze_result_t(ZE_APICALL *)(ze_graph_handle_t hGraph);
using zeGraphDumpContentsExpFP = ze_result_t(ZE_APICALL *)(ze_graph_handle_t hGraph, const char *filePath, void *pNext);

struct GraphApi {
    zeGraphCreateExpFP graphCreate = nullptr;
    zeCommandListBeginGraphCaptureExpFP commandListBeginGraphCapture = nullptr;
    zeCommandListBeginCaptureIntoGraphExpFP commandListBeginCaptureIntoGraph = nullptr;
    zeCommandListEndGraphCaptureExpFP commandListEndGraphCapture = nullptr;
    zeCommandListInstantiateGraphExpFP commandListInstantiateGraph = nullptr;
    zeCommandListAppendGraphExpFP commandListAppendGraph = nullptr;
    zeGraphDestroyExpFP graphDestroy = nullptr;
    zeExecutableGraphDestroyExpFP executableGraphDestroy = nullptr;

    zeCommandListIsGraphCaptureEnabledExpFP commandListIsGraphCaptureEnabled = nullptr;
    zeGraphIsEmptyExpFP graphIsEmpty = nullptr;
    zeGraphDumpContentsExpFP graphDumpContents = nullptr;

    bool valid() const {
        return graphCreate && commandListBeginGraphCapture && commandListBeginCaptureIntoGraph && commandListEndGraphCapture && commandListInstantiateGraph && commandListAppendGraph && graphDestroy && executableGraphDestroy && commandListIsGraphCaptureEnabled && graphIsEmpty && graphDumpContents;
    }
};

GraphApi loadGraphApi(ze_driver_handle_t driver) {
    GraphApi ret;
    EXPECT_ZE_RESULT_SUCCESS(zeDriverGetExtensionFunctionAddress(driver, "zeGraphCreateExp", reinterpret_cast<void **>(&ret.graphCreate)));
    EXPECT_ZE_RESULT_SUCCESS(zeDriverGetExtensionFunctionAddress(driver, "zeCommandListBeginGraphCaptureExp", reinterpret_cast<void **>(&ret.commandListBeginGraphCapture)));
    EXPECT_ZE_RESULT_SUCCESS(zeDriverGetExtensionFunctionAddress(driver, "zeCommandListBeginCaptureIntoGraphExp", reinterpret_cast<void **>(&ret.commandListBeginCaptureIntoGraph)));
    EXPECT_ZE_RESULT_SUCCESS(zeDriverGetExtensionFunctionAddress(driver, "zeCommandListEndGraphCaptureExp", reinterpret_cast<void **>(&ret.commandListEndGraphCapture)));
    EXPECT_ZE_RESULT_SUCCESS(zeDriverGetExtensionFunctionAddress(driver, "zeCommandListInstantiateGraphExp", reinterpret_cast<void **>(&ret.commandListInstantiateGraph)));
    EXPECT_ZE_RESULT_SUCCESS(zeDriverGetExtensionFunctionAddress(driver, "zeCommandListAppendGraphExp", reinterpret_cast<void **>(&ret.commandListAppendGraph)));
    EXPECT_ZE_RESULT_SUCCESS(zeDriverGetExtensionFunctionAddress(driver, "zeGraphDestroyExp", reinterpret_cast<void **>(&ret.graphDestroy)));
    EXPECT_ZE_RESULT_SUCCESS(zeDriverGetExtensionFunctionAddress(driver, "zeExecutableGraphDestroyExp", reinterpret_cast<void **>(&ret.executableGraphDestroy)));

    EXPECT_ZE_RESULT_SUCCESS(zeDriverGetExtensionFunctionAddress(driver, "zeCommandListIsGraphCaptureEnabledExp", reinterpret_cast<void **>(&ret.commandListIsGraphCaptureEnabled)));
    EXPECT_ZE_RESULT_SUCCESS(zeDriverGetExtensionFunctionAddress(driver, "zeGraphIsEmptyExp", reinterpret_cast<void **>(&ret.graphIsEmpty)));
    EXPECT_ZE_RESULT_SUCCESS(zeDriverGetExtensionFunctionAddress(driver, "zeGraphDumpContentsExp", reinterpret_cast<void **>(&ret.graphDumpContents)));

    return ret;
}

struct ImmCmdListCreateInfo {
    ze_device_handle_t hDevice;
    ze_command_queue_desc_t desc;
};

using ImmCmdListInfos = std::unordered_map<ze_command_list_handle_t, ImmCmdListCreateInfo>;

struct RecordedGraphBase {
    virtual ~RecordedGraphBase() = default;
    virtual void instantiateExecutableGraphs(int num, std::function<std::unique_ptr<RecordedGraphBase>()> rerecordFunc) = 0;
};

template <typename ParentT>
struct RecordGraphImplBase : RecordedGraphBase {
    void beginRecording(ze_context_handle_t ctxArg, GraphApi graphApiArg, ze_command_list_handle_t rootCmdListArg, const ImmCmdListInfos &knownImmCmdListsArg) {
        this->ctx = ctxArg;
        this->graphApi = graphApiArg;
        this->root = rootCmdListArg;
        this->knownImmCmdLists = &knownImmCmdListsArg;
        thisGraph()->beginRecordingImpl();
    }

    void endRecording() {
        thisGraph()->endRecordingImpl();
    }

    void fork(ze_command_list_handle_t forkTo) {
        thisGraph()->forkImpl(forkTo);
    }

    void join() {
        thisGraph()->joinImpl();
    }

    ParentT *thisGraph() {
        return static_cast<ParentT *>(this);
    }

    ze_command_list_handle_t getAppendTarget(int parentLevel = 0) {
        return *(cmdListStack.rbegin() + parentLevel);
    }

  protected:
    ze_context_handle_t ctx = nullptr;
    GraphApi graphApi;
    ze_command_list_handle_t root = nullptr;
    std::vector<ze_command_list_handle_t> cmdListStack;
    const ImmCmdListInfos *knownImmCmdLists;
};

template <bool emulated>
struct RecordedGraph;

// Real graph - ze_graph_handle_t
template <>
struct RecordedGraph<false> : RecordGraphImplBase<RecordedGraph<false>> {
    ze_graph_handle_t graph = nullptr;
    std::vector<ze_executable_graph_handle_t> instantiatedGraphs;

    RecordedGraph() = default;
    RecordedGraph(const RecordedGraph &) = delete;
    RecordedGraph(RecordedGraph &&) = delete;
    RecordedGraph &operator=(const RecordedGraph &) = delete;
    RecordedGraph &operator=(RecordedGraph &&) = delete;
    ~RecordedGraph() override {
        for (auto execGraph : instantiatedGraphs) {
            EXPECT_ZE_RESULT_SUCCESS(graphApi.executableGraphDestroy(execGraph));
        }
        EXPECT_ZE_RESULT_SUCCESS(graphApi.graphDestroy(graph));
    }

    void beginRecordingImpl() {
        cmdListStack.push_back(root);

        graphApi.graphCreate(ctx, &graph, nullptr);
        graphApi.commandListBeginCaptureIntoGraph(root, graph, nullptr);
    }

    void endRecordingImpl() {
        graphApi.commandListEndGraphCapture(root, nullptr, nullptr);
    }

    void forkImpl(ze_command_list_handle_t forkTo) {
        cmdListStack.push_back(forkTo);
    }

    void joinImpl() {
        cmdListStack.pop_back();
    }

    void instantiateExecutableGraphs(int num, [[maybe_unused]] std::function<std::unique_ptr<RecordedGraphBase>()> rerecordFunc) override {
        instantiatedGraphs.reserve(instantiatedGraphs.size() + num);
        for (int i = 0; i < num; ++i) {
            ze_executable_graph_handle_t execGraph = nullptr;
            graphApi.commandListInstantiateGraph(graph, &execGraph, nullptr);
            instantiatedGraphs.push_back(execGraph);
        }
    }
};

// Emulated graph - regular commandlists
template <>
struct RecordedGraph<true> : RecordGraphImplBase<RecordedGraph<true>> {
    ze_command_list_handle_t parent;
    std::vector<ze_command_list_handle_t> resources;
    std::vector<std::unique_ptr<RecordedGraphBase>> instantiatedGraphs;

    RecordedGraph() = default;
    RecordedGraph(const RecordedGraph &) = delete;
    RecordedGraph(RecordedGraph &&) = delete;
    RecordedGraph &operator=(const RecordedGraph &) = delete;
    RecordedGraph &operator=(RecordedGraph &&) = delete;
    ~RecordedGraph() override {
        for (auto cmdlist : resources) {
            EXPECT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdlist));
        }
    }

    void beginRecordingImpl() {
        const auto &cmdListInfo = (*knownImmCmdLists).at(root);
        ze_command_list_handle_t newRegularCmdList = nullptr;
        ze_command_list_desc_t cmdListDesc = {ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC, nullptr};
        cmdListDesc.commandQueueGroupOrdinal = cmdListInfo.desc.ordinal;
        cmdListDesc.flags |= (0 != (cmdListInfo.desc.flags & ZE_COMMAND_QUEUE_FLAG_IN_ORDER)) ? static_cast<ze_command_list_flags_t>(ZE_COMMAND_LIST_FLAG_IN_ORDER) : 0U;
        EXPECT_ZE_RESULT_SUCCESS(zeCommandListCreate(ctx, cmdListInfo.hDevice, &cmdListDesc, &newRegularCmdList));
        cmdListStack.push_back(newRegularCmdList);
    }

    void endRecordingImpl() {
        EXPECT_ZE_RESULT_SUCCESS(zeCommandListClose(*cmdListStack.begin()));
    }

    void forkImpl(ze_command_list_handle_t forkTo) {
        const auto &cmdListInfo = (*knownImmCmdLists).at(forkTo);
        ze_command_list_handle_t newRegularCmdList = nullptr;
        ze_command_list_desc_t cmdListDesc = {ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC, nullptr};
        cmdListDesc.commandQueueGroupOrdinal = cmdListInfo.desc.ordinal;
        cmdListDesc.flags |= (0 != (cmdListInfo.desc.flags & ZE_COMMAND_QUEUE_FLAG_IN_ORDER)) ? static_cast<ze_command_list_flags_t>(ZE_COMMAND_LIST_FLAG_IN_ORDER) : 0U;
        EXPECT_ZE_RESULT_SUCCESS(zeCommandListCreate(ctx, cmdListInfo.hDevice, &cmdListDesc, &newRegularCmdList));
        cmdListStack.push_back(newRegularCmdList);
        resources.push_back(newRegularCmdList);
    }

    void joinImpl() {
        EXPECT_ZE_RESULT_SUCCESS(zeCommandListClose(*cmdListStack.rbegin()));
        cmdListStack.pop_back();
    }

    void instantiateExecutableGraphs([[maybe_unused]] int num, std::function<std::unique_ptr<RecordedGraphBase>()> rerecordFunc) override {
        if (num <= 1) {
            return; // NOOP, recorded command lists already contain native commands
        }
        instantiatedGraphs.reserve(num - 1);
        for (int i = 1; i < num; ++i) {
            // instantiating graph in emulation mode means rerecording of all commands for each extra (>1) insantiation
            instantiatedGraphs.push_back(rerecordFunc());
        }
    }
};

struct ForkConfig {
    ze_event_handle_t forkEvent = nullptr;
    ze_event_handle_t joinEvent = nullptr;
    ze_command_list_handle_t target = nullptr;
};

using RecordGraphConfigStatus = TestResult;

struct RecordGraphConfig final {
    RecordGraphConfig(const RecordGraphArguments &arguments, LevelZero &levelzero) : l0env{levelzero} {
        ze_event_pool_desc_t eventPoolDesc = {ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr};
        eventPoolDesc.count = static_cast<uint32_t>(arguments.nLvls * 2);
        eventPoolDesc.count = eventPoolDesc.count ? eventPoolDesc.count : 1; // at least one
        EXPECT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.device, &l0env.eventPool));
        ze_event_desc_t eventDesc = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr};
        eventDesc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
        eventDesc.wait = ZE_EVENT_SCOPE_FLAG_DEVICE;

        this->numForksPerLevel = static_cast<int>(arguments.nForksInLvl);
        this->numCommandSetsPerLevel = static_cast<int>(arguments.nCmdSetsInLvl);
        this->numKernelsInSet = static_cast<int>(arguments.nAppendKern);
        this->numCopiesInSet = static_cast<int>(arguments.nAppendCopy);

        this->copySize = 4096U;
        ze_device_mem_alloc_desc_t devMemDesc{ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC, nullptr};
        EXPECT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &devMemDesc, this->copySize, 16, levelzero.device, &this->copyFrom));
        EXPECT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &devMemDesc, this->copySize, 16, levelzero.device, &this->copyTo));

        EXPECT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &levelzero.commandQueueDesc, &this->rootCmdlist));
        l0env.cmdListInfos[this->rootCmdlist] = {levelzero.device, levelzero.commandQueueDesc};
        this->forkLevelInfo.resize(arguments.nLvls);
        for (int i = 0; i < arguments.nLvls; ++i) {
            eventDesc.index = i * 2;
            EXPECT_ZE_RESULT_SUCCESS(zeEventCreate(this->l0env.eventPool, &eventDesc, &this->forkLevelInfo[i].forkEvent));
            eventDesc.index = i * 2 + 1;
            EXPECT_ZE_RESULT_SUCCESS(zeEventCreate(this->l0env.eventPool, &eventDesc, &this->forkLevelInfo[i].joinEvent));

            EXPECT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &levelzero.commandQueueDesc, &this->forkLevelInfo[i].target));
            l0env.cmdListInfos[this->forkLevelInfo[i].target] = {levelzero.device, levelzero.commandQueueDesc};
        }

        // Create kernel
        auto spirvModule = FileHelper::loadBinaryFile("graph_api_benchmark_kernel_assign.spv");
        if (spirvModule.size() == 0) {
            this->status = RecordGraphConfigStatus::KernelNotFound;
            return;
        }
        ze_module_desc_t moduleDesc{ZE_STRUCTURE_TYPE_MODULE_DESC};
        moduleDesc.format = ZE_MODULE_FORMAT_IL_SPIRV;
        moduleDesc.pInputModule = reinterpret_cast<const uint8_t *>(spirvModule.data());
        moduleDesc.inputSize = spirvModule.size();
        EXPECT_ZE_RESULT_SUCCESS(zeModuleCreate(levelzero.context, levelzero.device, &moduleDesc, &l0env.module, nullptr));
        ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
        kernelDesc.pKernelName = "kernel_assign";
        EXPECT_ZE_RESULT_SUCCESS(zeKernelCreate(l0env.module, &kernelDesc, &this->kernel));

        this->kernelGroupCount = {8, 8, 1};
        this->status = RecordGraphConfigStatus::Success;
    }

    ~RecordGraphConfig() {
        for (auto &level : this->forkLevelInfo) {
            EXPECT_ZE_RESULT_SUCCESS(zeEventDestroy(level.forkEvent));
            EXPECT_ZE_RESULT_SUCCESS(zeEventDestroy(level.joinEvent));
            EXPECT_ZE_RESULT_SUCCESS(zeCommandListDestroy(level.target));
        }

        EXPECT_ZE_RESULT_SUCCESS(zeCommandListDestroy(this->rootCmdlist));

        EXPECT_ZE_RESULT_SUCCESS(zeMemFree(l0env.envBase.context, this->copyFrom));
        EXPECT_ZE_RESULT_SUCCESS(zeMemFree(l0env.envBase.context, this->copyTo));
        EXPECT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(this->l0env.eventPool));
        EXPECT_ZE_RESULT_SUCCESS(zeKernelDestroy(this->kernel));
        EXPECT_ZE_RESULT_SUCCESS(zeModuleDestroy(l0env.module));
    }

    RecordGraphConfig(const RecordGraphConfig &) = delete;
    RecordGraphConfig &operator=(const RecordGraphConfig &) = delete;
    RecordGraphConfig(RecordGraphConfig &&) = delete;
    RecordGraphConfig &operator=(RecordGraphConfig &&) = delete;

    ze_command_list_handle_t rootCmdlist = nullptr;
    std::vector<ForkConfig> forkLevelInfo;

    int numForksPerLevel = 0;
    int numCommandSetsPerLevel = 1;
    int numKernelsInSet = 1;
    int numCopiesInSet = 1;

    ze_kernel_handle_t kernel = nullptr;
    ze_group_count_t kernelGroupCount = {1, 1, 1};

    void *copyFrom = nullptr;
    void *copyTo = nullptr;
    size_t copySize = 0;

    struct {
        LevelZero &envBase;
        ze_event_pool_handle_t eventPool = nullptr;
        ze_module_handle_t module = nullptr;

        ImmCmdListInfos cmdListInfos;
    } l0env;

    RecordGraphConfigStatus status = RecordGraphConfigStatus::Error;
};

template <bool emulated>
void recordGraphLevel(int level, const LevelZero &levelzero, const GraphApi &graphApi, const RecordGraphConfig &cfg, RecordedGraph<emulated> &graph) {
    ze_event_handle_t forkEvent = cfg.forkLevelInfo[level].forkEvent;
    ze_event_handle_t waitEvent = nullptr;
    if (level > 0) {
        waitEvent = cfg.forkLevelInfo[level - 1].forkEvent;
    }

    int cmdId = 0;
    int totalCommands = cfg.numCommandSetsPerLevel * (cfg.numKernelsInSet + cfg.numCopiesInSet);
    int forkToCommandRatio = std::numeric_limits<int>::max();
    int numForksToCreate = 0;
    if ((cfg.numForksPerLevel > 0) && (level + 1 < static_cast<int>(cfg.forkLevelInfo.size()))) {
        forkToCommandRatio = totalCommands / cfg.numForksPerLevel;
        numForksToCreate = cfg.numForksPerLevel;
    }

    int forkSpan = 0;
    int numForks = 0;
    for (int i = 0; i < cfg.numCommandSetsPerLevel; ++i) {
        for (int k = 0; k < cfg.numKernelsInSet; ++k, ++cmdId, ++forkSpan) {
            EXPECT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(cfg.kernel, 0, sizeof(void *), (0 != (k & 1)) ? &cfg.copyTo : &cfg.copyFrom));
            EXPECT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(cfg.kernel, 1, sizeof(void *), (0 != (k & 1)) ? &cfg.copyFrom : &cfg.copyTo));
            if ((forkSpan == forkToCommandRatio) || (numForksToCreate - numForks >= totalCommands - cmdId)) {
                EXPECT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(graph.getAppendTarget(), cfg.kernel, &cfg.kernelGroupCount, forkEvent, waitEvent ? 1 : 0, waitEvent ? &waitEvent : nullptr));
                waitEvent = cfg.forkLevelInfo[level + 1].joinEvent;

                graph.fork(cfg.forkLevelInfo[level + 1].target);
                recordGraphLevel(level + 1, levelzero, graphApi, cfg, graph);
                ++numForks;
                forkSpan = 0;
                graph.join();
            } else {
                EXPECT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(graph.getAppendTarget(), cfg.kernel, &cfg.kernelGroupCount, nullptr, waitEvent ? 1 : 0, waitEvent ? &waitEvent : nullptr));
                waitEvent = nullptr;
            }
        }
        for (int c = 0; c < cfg.numCopiesInSet; ++c, ++cmdId, ++forkSpan) {
            if ((forkSpan == forkToCommandRatio) || (numForksToCreate - numForks >= totalCommands - cmdId)) {
                EXPECT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(graph.getAppendTarget(), cfg.copyTo, cfg.copyFrom, cfg.copySize, forkEvent, waitEvent ? 1 : 0, waitEvent ? &waitEvent : nullptr));
                waitEvent = cfg.forkLevelInfo[level + 1].joinEvent;

                graph.fork(cfg.forkLevelInfo[level + 1].target);
                recordGraphLevel(level + 1, levelzero, graphApi, cfg, graph);
                ++numForks;
                forkSpan = 0;
                graph.join();
            } else {
                EXPECT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(graph.getAppendTarget(), cfg.copyTo, cfg.copyFrom, cfg.copySize, nullptr, waitEvent ? 1 : 0, waitEvent ? &waitEvent : nullptr));
                waitEvent = nullptr;
            }
        }
    }
    if ((level > 0) || waitEvent) { // trailing join
        EXPECT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(graph.getAppendTarget(), cfg.copyTo, cfg.copyFrom, cfg.copySize,
                                                               (level > 0) ? cfg.forkLevelInfo[level].joinEvent : nullptr, waitEvent ? 1 : 0, waitEvent ? &waitEvent : nullptr));
    }
}

template <bool emulated>
std::unique_ptr<RecordedGraphBase> recordGraph(const LevelZero &levelzero, const GraphApi &graphApi, const RecordGraphConfig &cfg) {
    auto ret = std::make_unique<RecordedGraph<emulated>>();
    ret->beginRecording(levelzero.context, graphApi, cfg.rootCmdlist, cfg.l0env.cmdListInfos);

    recordGraphLevel(0, levelzero, graphApi, cfg, *ret);

    ret->endRecording();

    return std::unique_ptr<RecordedGraphBase>{ret.release()};
}

} // namespace

static TestResult run(const RecordGraphArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    LevelZero levelzero;
    levelzero.createSubDevices(false, true);

    decltype(&recordGraph<false>) recordFunc = recordGraph<false>;
    if (arguments.emulate) {
        recordFunc = recordGraph<true>;
    }

    GraphApi graphApi = loadGraphApi(levelzero.driver);

    RecordGraphConfig cfg{arguments, levelzero};
    if (TestResult status = cfg.status; status != TestResult::Success) {
        return status;
    }

    auto recordGraphOnce = [&]() { return recordFunc(levelzero, graphApi, cfg); };

    Timer timer;
    auto startMeasureIfInactive = [&timer](bool &isActive) {
        if (!isActive) {
            isActive = true;
            timer.measureStart();
        }
    };
    auto stopMeasureIfActive = [&timer](bool &isActive) {
        auto out = decltype(timer.get()){};
        if (isActive) {
            timer.measureEnd();
            out = timer.get();
            isActive = false;
        }
        return out;
    };

    for (auto i = 0u; i < arguments.iterations; i++) {
        decltype(timer.get()) firstPartTime = {};
        bool isMeasuring = false;

        if (arguments.mRec) {
            startMeasureIfInactive(isMeasuring);
        }

        auto graph = recordGraphOnce();

        if (arguments.mInst) {
            startMeasureIfInactive(isMeasuring);
        } else {
            firstPartTime = stopMeasureIfActive(isMeasuring);
        }

        graph->instantiateExecutableGraphs(static_cast<int>(arguments.nInstantiations), recordGraphOnce);

        if (arguments.mDest) {
            startMeasureIfInactive(isMeasuring);
            graph.reset();
        }

        auto secondPartTime = stopMeasureIfActive(isMeasuring);
        statistics.pushValue(firstPartTime + secondPartTime, typeSelector.getUnit(), typeSelector.getType());
    }

    return TestResult::Success;
};

[[maybe_unused]] static RegisterTestCaseImplementation<RecordGraph> registerTestCase(run, Api::L0, true);
