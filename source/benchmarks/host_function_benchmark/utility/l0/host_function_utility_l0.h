#pragma once

#include <cstdint>
#include <level_zero/ze_api.h>

#ifndef ZE_CALLBACK
#if defined(_WIN32)
#define ZE_CALLBACK __cdecl
#else
#define ZE_CALLBACK
#endif // defined(_WIN32)
#endif // ZE_CALLBACK

using zeCommandListAppendHostFunctionPF = ze_result_t(ZE_APICALL *)(ze_command_list_handle_t hCommandList,
                                                                    void *pHostFunction,
                                                                    void *pUserData,
                                                                    void *pNext,
                                                                    ze_event_handle_t hSignalEvent,
                                                                    uint32_t numWaitEvents,
                                                                    ze_event_handle_t *phWaitEvents);

struct HostFunctionApi {
    zeCommandListAppendHostFunctionPF commandListAppendHostFunction = nullptr;
};

HostFunctionApi loadHostFunctionApi(ze_driver_handle_t driver) {
    HostFunctionApi ret;
    zeDriverGetExtensionFunctionAddress(driver, "zeCommandListAppendHostFunction", reinterpret_cast<void **>(&ret.commandListAppendHostFunction));
    return ret;
}

void ZE_CALLBACK emptyHostFunction([[maybe_unused]] void *pUserData) {
}

struct HostFunctionCountEvenArgs {
    int *array = nullptr;
    uint32_t n = 0;
    uint32_t result = 0;
};

void ZE_CALLBACK countEvenHostFunction(void *pUserData) {
    auto args = static_cast<HostFunctionCountEvenArgs *>(pUserData);

    for (auto i = 0u; i < args->n; i++) {
        if (args->array[i] % 2 == 0) {
            args->result++;
        }
    }
}

using HostFunctionT = void(ZE_CALLBACK *)(void *pUserData);

struct HostFunctions {
    HostFunctionT emptyHostFunction = emptyHostFunction;
    void *emptyHostFunctionUserData = nullptr;
};

HostFunctions getHostFunctions() {
    HostFunctions ret;
    return ret;
}
