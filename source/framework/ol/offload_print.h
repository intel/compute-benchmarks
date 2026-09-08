/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <OffloadAPI.h>
#include <string_view>

namespace OL {

// Ideally the generated <OffloadPrint.hpp> would provide these conversions,
// but its llvm::raw_ostream interface introduces a problematic LLVM Support
// link dependency for the benchmark framework.

#define OL_ENUM_TO_STRING_CASE(enumerator) \
    case enumerator:                       \
        return #enumerator

inline constexpr std::string_view toString(ol_alloc_type_t value) {
    switch (value) {
        OL_ENUM_TO_STRING_CASE(OL_ALLOC_TYPE_HOST);
        OL_ENUM_TO_STRING_CASE(OL_ALLOC_TYPE_DEVICE);
        OL_ENUM_TO_STRING_CASE(OL_ALLOC_TYPE_MANAGED);
    default:
        return "unknown enumerator";
    }
}

inline constexpr std::string_view toString(ol_context_info_t value) {
    switch (value) {
        OL_ENUM_TO_STRING_CASE(OL_CONTEXT_INFO_NUM_DEVICES);
        OL_ENUM_TO_STRING_CASE(OL_CONTEXT_INFO_DEVICES);
        OL_ENUM_TO_STRING_CASE(OL_CONTEXT_INFO_PLATFORM);
    default:
        return "unknown enumerator";
    }
}

inline constexpr std::string_view
toString(ol_device_fp_capability_flag_t value) {
    switch (value) {
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_FP_CAPABILITY_FLAG_CORRECTLY_ROUNDED_DIVIDE_SQRT);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_FP_CAPABILITY_FLAG_ROUND_TO_NEAREST);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_FP_CAPABILITY_FLAG_ROUND_TO_ZERO);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_FP_CAPABILITY_FLAG_ROUND_TO_INF);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_FP_CAPABILITY_FLAG_INF_NAN);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_FP_CAPABILITY_FLAG_DENORM);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_FP_CAPABILITY_FLAG_FMA);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_FP_CAPABILITY_FLAG_SOFT_FLOAT);
    default:
        return "unknown enumerator";
    }
}

inline constexpr std::string_view toString(ol_device_info_t value) {
    switch (value) {
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_INFO_TYPE);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_INFO_PLATFORM);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_INFO_NAME);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_INFO_PRODUCT_NAME);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_INFO_UID);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_INFO_VENDOR);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_INFO_DRIVER_VERSION);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_INFO_MAX_WORK_GROUP_SIZE);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_INFO_MAX_WORK_GROUP_SIZE_PER_DIMENSION);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_INFO_MAX_WORK_SIZE);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_INFO_MAX_WORK_SIZE_PER_DIMENSION);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_INFO_VENDOR_ID);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_INFO_NUM_COMPUTE_UNITS);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_INFO_MAX_CLOCK_FREQUENCY);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_INFO_MEMORY_CLOCK_RATE);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_INFO_ADDRESS_BITS);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_INFO_MAX_MEM_ALLOC_SIZE);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_INFO_GLOBAL_MEM_SIZE);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_INFO_WORK_GROUP_LOCAL_MEM_SIZE);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_INFO_NUM_LANES);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_INFO_SINGLE_FP_CONFIG);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_INFO_DOUBLE_FP_CONFIG);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_INFO_HALF_FP_CONFIG);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_INFO_NATIVE_VECTOR_WIDTH_CHAR);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_INFO_NATIVE_VECTOR_WIDTH_SHORT);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_INFO_NATIVE_VECTOR_WIDTH_INT);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_INFO_NATIVE_VECTOR_WIDTH_LONG);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_INFO_NATIVE_VECTOR_WIDTH_FLOAT);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_INFO_NATIVE_VECTOR_WIDTH_DOUBLE);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_INFO_NATIVE_VECTOR_WIDTH_HALF);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_INFO_SINGLE_FP_SUPPORT);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_INFO_DOUBLE_FP_SUPPORT);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_INFO_HALF_FP_SUPPORT);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_INFO_COOPERATIVE_LAUNCH_SUPPORT);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_INFO_DRIVER_ID);
    default:
        return "unknown enumerator";
    }
}

inline constexpr std::string_view toString(ol_device_type_t value) {
    switch (value) {
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_TYPE_DEFAULT);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_TYPE_ALL);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_TYPE_GPU);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_TYPE_CPU);
        OL_ENUM_TO_STRING_CASE(OL_DEVICE_TYPE_HOST);
    default:
        return "unknown enumerator";
    }
}

inline constexpr std::string_view toString(ol_errc_t value) {
    switch (value) {
        OL_ENUM_TO_STRING_CASE(OL_ERRC_SUCCESS);
        OL_ENUM_TO_STRING_CASE(OL_ERRC_UNKNOWN);
        OL_ENUM_TO_STRING_CASE(OL_ERRC_HOST_IO);
        OL_ENUM_TO_STRING_CASE(OL_ERRC_INVALID_BINARY);
        OL_ENUM_TO_STRING_CASE(OL_ERRC_INVALID_NULL_POINTER);
        OL_ENUM_TO_STRING_CASE(OL_ERRC_INVALID_ARGUMENT);
        OL_ENUM_TO_STRING_CASE(OL_ERRC_NOT_FOUND);
        OL_ENUM_TO_STRING_CASE(OL_ERRC_OUT_OF_RESOURCES);
        OL_ENUM_TO_STRING_CASE(OL_ERRC_INVALID_SIZE);
        OL_ENUM_TO_STRING_CASE(OL_ERRC_INVALID_ENUMERATION);
        OL_ENUM_TO_STRING_CASE(OL_ERRC_HOST_TOOL_NOT_FOUND);
        OL_ENUM_TO_STRING_CASE(OL_ERRC_INVALID_VALUE);
        OL_ENUM_TO_STRING_CASE(OL_ERRC_UNIMPLEMENTED);
        OL_ENUM_TO_STRING_CASE(OL_ERRC_UNSUPPORTED);
        OL_ENUM_TO_STRING_CASE(OL_ERRC_ASSEMBLE_FAILURE);
        OL_ENUM_TO_STRING_CASE(OL_ERRC_COMPILE_FAILURE);
        OL_ENUM_TO_STRING_CASE(OL_ERRC_LINK_FAILURE);
        OL_ENUM_TO_STRING_CASE(OL_ERRC_BACKEND_FAILURE);
        OL_ENUM_TO_STRING_CASE(OL_ERRC_UNINITIALIZED);
        OL_ENUM_TO_STRING_CASE(OL_ERRC_INVALID_NULL_HANDLE);
        OL_ENUM_TO_STRING_CASE(OL_ERRC_INVALID_PLATFORM);
        OL_ENUM_TO_STRING_CASE(OL_ERRC_INVALID_DEVICE);
        OL_ENUM_TO_STRING_CASE(OL_ERRC_INVALID_QUEUE);
        OL_ENUM_TO_STRING_CASE(OL_ERRC_INVALID_EVENT);
        OL_ENUM_TO_STRING_CASE(OL_ERRC_INVALID_CONTEXT);
        OL_ENUM_TO_STRING_CASE(OL_ERRC_SYMBOL_KIND);
    default:
        return "unknown enumerator";
    }
}

inline constexpr std::string_view toString(ol_event_flags_t value) {
    switch (value) {
        OL_ENUM_TO_STRING_CASE(OL_EVENT_FLAGS_NONE);
        OL_ENUM_TO_STRING_CASE(OL_EVENT_FLAGS_ENABLE_PROFILING);
    default:
        return "unknown enumerator";
    }
}

inline constexpr std::string_view toString(ol_event_info_t value) {
    switch (value) {
        OL_ENUM_TO_STRING_CASE(OL_EVENT_INFO_QUEUE);
        OL_ENUM_TO_STRING_CASE(OL_EVENT_INFO_IS_COMPLETE);
    default:
        return "unknown enumerator";
    }
}

inline constexpr std::string_view
toString(ol_kernel_launch_prop_type_t value) {
    switch (value) {
        OL_ENUM_TO_STRING_CASE(OL_KERNEL_LAUNCH_PROP_TYPE_NONE);
        OL_ENUM_TO_STRING_CASE(OL_KERNEL_LAUNCH_PROP_TYPE_IS_COOPERATIVE);
    default:
        return "unknown enumerator";
    }
}

inline constexpr std::string_view toString(ol_mem_info_t value) {
    switch (value) {
        OL_ENUM_TO_STRING_CASE(OL_MEM_INFO_DEVICE);
        OL_ENUM_TO_STRING_CASE(OL_MEM_INFO_BASE);
        OL_ENUM_TO_STRING_CASE(OL_MEM_INFO_SIZE);
        OL_ENUM_TO_STRING_CASE(OL_MEM_INFO_TYPE);
    default:
        return "unknown enumerator";
    }
}

inline constexpr std::string_view toString(ol_mem_migration_flag_t value) {
    switch (value) {
        OL_ENUM_TO_STRING_CASE(OL_MEM_MIGRATION_FLAG_HOST_TO_DEVICE);
        OL_ENUM_TO_STRING_CASE(OL_MEM_MIGRATION_FLAG_DEVICE_TO_HOST);
    default:
        return "unknown enumerator";
    }
}

inline constexpr std::string_view toString(ol_memory_register_flag_t value) {
    switch (value) {
        OL_ENUM_TO_STRING_CASE(OL_MEMORY_REGISTER_FLAG_LOCK_MEMORY);
        OL_ENUM_TO_STRING_CASE(OL_MEMORY_REGISTER_FLAG_UNLOCK_MEMORY);
    default:
        return "unknown enumerator";
    }
}

inline constexpr std::string_view toString(ol_platform_backend_t value) {
    switch (value) {
        OL_ENUM_TO_STRING_CASE(OL_PLATFORM_BACKEND_UNKNOWN);
        OL_ENUM_TO_STRING_CASE(OL_PLATFORM_BACKEND_CUDA);
        OL_ENUM_TO_STRING_CASE(OL_PLATFORM_BACKEND_AMDGPU);
        OL_ENUM_TO_STRING_CASE(OL_PLATFORM_BACKEND_LEVEL_ZERO);
        OL_ENUM_TO_STRING_CASE(OL_PLATFORM_BACKEND_HOST);
    default:
        return "unknown enumerator";
    }
}

inline constexpr std::string_view toString(ol_platform_info_t value) {
    switch (value) {
        OL_ENUM_TO_STRING_CASE(OL_PLATFORM_INFO_NAME);
        OL_ENUM_TO_STRING_CASE(OL_PLATFORM_INFO_VENDOR_NAME);
        OL_ENUM_TO_STRING_CASE(OL_PLATFORM_INFO_VERSION);
        OL_ENUM_TO_STRING_CASE(OL_PLATFORM_INFO_BACKEND);
    default:
        return "unknown enumerator";
    }
}

inline constexpr std::string_view toString(ol_queue_info_t value) {
    switch (value) {
        OL_ENUM_TO_STRING_CASE(OL_QUEUE_INFO_DEVICE);
        OL_ENUM_TO_STRING_CASE(OL_QUEUE_INFO_CONTEXT);
        OL_ENUM_TO_STRING_CASE(OL_QUEUE_INFO_EMPTY);
    default:
        return "unknown enumerator";
    }
}

inline constexpr std::string_view toString(ol_symbol_info_t value) {
    switch (value) {
        OL_ENUM_TO_STRING_CASE(OL_SYMBOL_INFO_KIND);
        OL_ENUM_TO_STRING_CASE(OL_SYMBOL_INFO_GLOBAL_VARIABLE_ADDRESS);
        OL_ENUM_TO_STRING_CASE(OL_SYMBOL_INFO_GLOBAL_VARIABLE_SIZE);
        OL_ENUM_TO_STRING_CASE(OL_SYMBOL_INFO_NAME);
    default:
        return "unknown enumerator";
    }
}

inline constexpr std::string_view toString(ol_symbol_kind_t value) {
    switch (value) {
        OL_ENUM_TO_STRING_CASE(OL_SYMBOL_KIND_KERNEL);
        OL_ENUM_TO_STRING_CASE(OL_SYMBOL_KIND_GLOBAL_VARIABLE);
    default:
        return "unknown enumerator";
    }
}

#undef OL_ENUM_TO_STRING_CASE

} // namespace OL
