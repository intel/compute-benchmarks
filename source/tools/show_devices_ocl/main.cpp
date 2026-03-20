/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/intel_product/get_intel_product_ocl.h"
#include "framework/ocl/utility/error.h"

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#ifndef CL_VERSION_MAJOR
#define CL_VERSION_MAJOR(version) ((version) >> 22)
#endif
#ifndef CL_VERSION_MINOR
#define CL_VERSION_MINOR(version) (((version) >> 12) & 0x3ff)
#endif
#ifndef CL_VERSION_PATCH
#define CL_VERSION_PATCH(version) ((version) & 0xfff)
#endif

#if !defined(CL_VERSION_2_1) && !defined(CL_VERSION_3_0)
using cl_version = cl_uint;
#endif

#ifdef CL_NAME_VERSION_MAX_NAME_SIZE
std::string formatVersion(cl_version version) {
    std::ostringstream os;
    os << CL_VERSION_MAJOR(version) << '.' << CL_VERSION_MINOR(version) << '.' << CL_VERSION_PATCH(version);
    return os.str();
}
#endif

std::string formatBytes(uint64_t bytes) {
    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    double size = static_cast<double>(bytes);
    size_t idx = 0;
    while (size >= 1024.0 && idx < 4) {
        size /= 1024.0;
        ++idx;
    }
    std::ostringstream os;
    os << std::fixed << std::setprecision(idx == 0 ? 0 : 2) << size << ' ' << units[idx];
    return os.str();
}

bool isByteSizeParam(cl_device_info param) {
    switch (param) {
    case CL_DEVICE_GLOBAL_MEM_SIZE:
    case CL_DEVICE_GLOBAL_MEM_CACHE_SIZE:
    case CL_DEVICE_MAX_MEM_ALLOC_SIZE:
    case CL_DEVICE_LOCAL_MEM_SIZE:
    case CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE:
    case CL_DEVICE_MAX_PARAMETER_SIZE:
    case CL_DEVICE_PRINTF_BUFFER_SIZE:
    case CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE:
#ifdef CL_DEVICE_GLOBAL_VARIABLE_PREFERRED_TOTAL_SIZE
    case CL_DEVICE_GLOBAL_VARIABLE_PREFERRED_TOTAL_SIZE:
#endif
#ifdef CL_DEVICE_MAX_GLOBAL_VARIABLE_SIZE
    case CL_DEVICE_MAX_GLOBAL_VARIABLE_SIZE:
#endif
#ifdef CL_DEVICE_QUEUE_ON_DEVICE_PREFERRED_SIZE
    case CL_DEVICE_QUEUE_ON_DEVICE_PREFERRED_SIZE:
#endif
#ifdef CL_DEVICE_QUEUE_ON_DEVICE_MAX_SIZE
    case CL_DEVICE_QUEUE_ON_DEVICE_MAX_SIZE:
#endif
#ifdef CL_DEVICE_PIPE_MAX_PACKET_SIZE
    case CL_DEVICE_PIPE_MAX_PACKET_SIZE:
#endif
#ifdef CL_DEVICE_PREFERRED_PLATFORM_ATOMIC_ALIGNMENT
    case CL_DEVICE_PREFERRED_PLATFORM_ATOMIC_ALIGNMENT:
#endif
#ifdef CL_DEVICE_PREFERRED_GLOBAL_ATOMIC_ALIGNMENT
    case CL_DEVICE_PREFERRED_GLOBAL_ATOMIC_ALIGNMENT:
#endif
#ifdef CL_DEVICE_PREFERRED_LOCAL_ATOMIC_ALIGNMENT
    case CL_DEVICE_PREFERRED_LOCAL_ATOMIC_ALIGNMENT:
#endif
        return true;
    default:
        return false;
    }
}

enum class DeviceInfoValueType {
    Bool,
    Uint,
    Ulong,
    SizeT,
    String,
    BitfieldHex,
    SizeTArray,
    UintArray,
    UlongArray,
    NameVersionArray,
#ifdef CL_NAME_VERSION_MAX_NAME_SIZE
    Version,
#endif
    PlatformId,
    DeviceId,
    SingleFpConfig,
    CacheType,
    ExecCapabilities,
    QueueProperties,
    LocalMemType,
    HalfFpConfig,
    DeviceType,
    SvmCapabilities,
    DoubleFpConfig,
};

struct DeviceInfoDescriptor {
    cl_device_info param{};
    const char *name{};
    DeviceInfoValueType type{};
};

template <typename T>
T readScalar(const std::vector<unsigned char> &data) {
    T value{};
    std::memcpy(&value, data.data(), std::min(sizeof(T), data.size()));
    return value;
}

template <typename T>
std::vector<T> readVector(const std::vector<unsigned char> &data) {
    const size_t count = data.size() / sizeof(T);
    std::vector<T> values(count);
    if (count > 0) {
        std::memcpy(values.data(), data.data(), sizeof(T) * count);
    }
    return values;
}

#ifdef CL_NAME_VERSION_MAX_NAME_SIZE
std::string formatVersion(cl_version version) {
    std::ostringstream os;
    os << CL_VERSION_MAJOR(version) << '.' << CL_VERSION_MINOR(version) << '.' << CL_VERSION_PATCH(version);
    return os.str();
}
#endif

void printDeviceInfoValue(const std::string &indent, cl_device_id device, const DeviceInfoDescriptor &desc) {
    size_t paramSize{};
    cl_int retVal = clGetDeviceInfo(device, desc.param, 0, nullptr, &paramSize);
    if (retVal != CL_SUCCESS) {
        std::cout << indent << desc.name << ": <" << oclErrorToString(retVal) << ">\n";
        return;
    }

    std::vector<unsigned char> buffer(paramSize);
    retVal = clGetDeviceInfo(device, desc.param, paramSize, buffer.data(), nullptr);
    if (retVal != CL_SUCCESS) {
        std::cout << indent << desc.name << ": <" << oclErrorToString(retVal) << ">\n";
        return;
    }

    std::cout << indent << desc.name << ": ";

    switch (desc.type) {
    case DeviceInfoValueType::Bool: {
        const auto value = readScalar<cl_bool>(buffer);
        std::cout << (value ? "CL_TRUE" : "CL_FALSE");
        break;
    }
    case DeviceInfoValueType::Uint: {
        const auto value = readScalar<cl_uint>(buffer);
        std::cout << value;
        break;
    }
    case DeviceInfoValueType::Ulong: {
        const auto value = readScalar<cl_ulong>(buffer);
        std::cout << value;
        if (isByteSizeParam(desc.param) && value > 0u) {
            std::cout << " (" << formatBytes(static_cast<uint64_t>(value)) << ")";
        }
        break;
    }
    case DeviceInfoValueType::SizeT: {
        const auto value = readScalar<size_t>(buffer);
        std::cout << value;
        if (isByteSizeParam(desc.param) && value > 0u) {
            std::cout << " (" << formatBytes(static_cast<uint64_t>(value)) << ")";
        }
        break;
    }
    case DeviceInfoValueType::String: {
        const auto *str = reinterpret_cast<const char *>(buffer.data());
        std::cout << str;
        break;
    }
    case DeviceInfoValueType::BitfieldHex: {
        cl_ulong value = 0u;
        std::memcpy(&value, buffer.data(), std::min(sizeof(value), buffer.size()));
        std::cout << std::hex << std::showbase << value << std::dec << std::noshowbase;
        break;
    }
    case DeviceInfoValueType::SingleFpConfig: {
        const auto value = readScalar<cl_device_fp_config>(buffer);
        bool printed = false;
        auto printFlag = [&](cl_device_fp_config flag, const char *name) {
            if (value & flag) {
                if (printed) {
                    std::cout << ' ';
                }
                std::cout << name;
                printed = true;
            }
        };
        printFlag(CL_FP_DENORM, "CL_FP_DENORM");
        printFlag(CL_FP_INF_NAN, "CL_FP_INF_NAN");
        printFlag(CL_FP_ROUND_TO_NEAREST, "CL_FP_ROUND_TO_NEAREST");
        printFlag(CL_FP_ROUND_TO_ZERO, "CL_FP_ROUND_TO_ZERO");
        printFlag(CL_FP_ROUND_TO_INF, "CL_FP_ROUND_TO_INF");
        printFlag(CL_FP_FMA, "CL_FP_FMA");
#ifdef CL_FP_SOFT_FLOAT
        printFlag(CL_FP_SOFT_FLOAT, "CL_FP_SOFT_FLOAT");
#endif
#ifdef CL_FP_CORRECTLY_ROUNDED_DIVIDE_SQRT
        printFlag(CL_FP_CORRECTLY_ROUNDED_DIVIDE_SQRT, "CL_FP_CORRECTLY_ROUNDED_DIVIDE_SQRT");
#endif
        if (!printed) {
            std::cout << static_cast<cl_uint>(value);
        }
        break;
    }
    case DeviceInfoValueType::HalfFpConfig: {
#ifdef CL_DEVICE_HALF_FP_CONFIG
        const auto value = readScalar<cl_device_fp_config>(buffer);
        bool printed = false;
        auto printFlag = [&](cl_device_fp_config flag, const char *name) {
            if (value & flag) {
                if (printed) {
                    std::cout << ' ';
                }
                std::cout << name;
                printed = true;
            }
        };
        printFlag(CL_FP_DENORM, "CL_FP_DENORM");
        printFlag(CL_FP_INF_NAN, "CL_FP_INF_NAN");
        printFlag(CL_FP_ROUND_TO_NEAREST, "CL_FP_ROUND_TO_NEAREST");
        printFlag(CL_FP_ROUND_TO_ZERO, "CL_FP_ROUND_TO_ZERO");
        printFlag(CL_FP_ROUND_TO_INF, "CL_FP_ROUND_TO_INF");
        printFlag(CL_FP_FMA, "CL_FP_FMA");
#ifdef CL_FP_SOFT_FLOAT
        printFlag(CL_FP_SOFT_FLOAT, "CL_FP_SOFT_FLOAT");
#endif
#ifdef CL_FP_CORRECTLY_ROUNDED_DIVIDE_SQRT
        printFlag(CL_FP_CORRECTLY_ROUNDED_DIVIDE_SQRT, "CL_FP_CORRECTLY_ROUNDED_DIVIDE_SQRT");
#endif
        if (!printed) {
            std::cout << static_cast<cl_uint>(value);
        }
#else
        std::cout << "<unsupported>";
#endif
        break;
    }
    case DeviceInfoValueType::DoubleFpConfig: {
#ifdef CL_DEVICE_DOUBLE_FP_CONFIG
        const auto value = readScalar<cl_device_fp_config>(buffer);
        bool printed = false;
        auto printFlag = [&](cl_device_fp_config flag, const char *name) {
            if (value & flag) {
                if (printed) {
                    std::cout << ' ';
                }
                std::cout << name;
                printed = true;
            }
        };
        printFlag(CL_FP_DENORM, "CL_FP_DENORM");
        printFlag(CL_FP_INF_NAN, "CL_FP_INF_NAN");
        printFlag(CL_FP_ROUND_TO_NEAREST, "CL_FP_ROUND_TO_NEAREST");
        printFlag(CL_FP_ROUND_TO_ZERO, "CL_FP_ROUND_TO_ZERO");
        printFlag(CL_FP_ROUND_TO_INF, "CL_FP_ROUND_TO_INF");
        printFlag(CL_FP_FMA, "CL_FP_FMA");
#ifdef CL_FP_SOFT_FLOAT
        printFlag(CL_FP_SOFT_FLOAT, "CL_FP_SOFT_FLOAT");
#endif
#ifdef CL_FP_CORRECTLY_ROUNDED_DIVIDE_SQRT
        printFlag(CL_FP_CORRECTLY_ROUNDED_DIVIDE_SQRT, "CL_FP_CORRECTLY_ROUNDED_DIVIDE_SQRT");
#endif
        if (!printed) {
            std::cout << static_cast<cl_uint>(value);
        }
#else
        std::cout << "<unsupported>";
#endif
        break;
    }
    case DeviceInfoValueType::CacheType: {
        const auto value = readScalar<cl_device_mem_cache_type>(buffer);
        switch (value) {
        case CL_NONE:
            std::cout << "CL_NONE";
            break;
        case CL_READ_ONLY_CACHE:
            std::cout << "CL_READ_ONLY_CACHE";
            break;
        case CL_READ_WRITE_CACHE:
            std::cout << "CL_READ_WRITE_CACHE";
            break;
        default:
            std::cout << static_cast<cl_uint>(value);
            break;
        }
        break;
    }
    case DeviceInfoValueType::LocalMemType: {
        const auto value = readScalar<cl_device_local_mem_type>(buffer);
        switch (value) {
        case CL_NONE:
            std::cout << "CL_NONE";
            break;
        case CL_LOCAL:
            std::cout << "CL_LOCAL";
            break;
        case CL_GLOBAL:
            std::cout << "CL_GLOBAL";
            break;
        default:
            std::cout << static_cast<cl_uint>(value);
            break;
        }
        break;
    }
    case DeviceInfoValueType::ExecCapabilities: {
        const auto value = readScalar<cl_device_exec_capabilities>(buffer);
        bool printed = false;
        auto printFlag = [&](cl_device_exec_capabilities flag, const char *name) {
            if (value & flag) {
                if (printed) {
                    std::cout << ' ';
                }
                std::cout << name;
                printed = true;
            }
        };
        printFlag(CL_EXEC_KERNEL, "CL_EXEC_KERNEL");
        printFlag(CL_EXEC_NATIVE_KERNEL, "CL_EXEC_NATIVE_KERNEL");
        if (!printed) {
            std::cout << static_cast<cl_uint>(value);
        }
        break;
    }
    case DeviceInfoValueType::QueueProperties: {
        const auto value = readScalar<cl_command_queue_properties>(buffer);
        bool printed = false;
        auto printFlag = [&](cl_command_queue_properties flag, const char *name) {
            if (value & flag) {
                if (printed) {
                    std::cout << ' ';
                }
                std::cout << name;
                printed = true;
            }
        };
        printFlag(CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, "CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE");
        printFlag(CL_QUEUE_PROFILING_ENABLE, "CL_QUEUE_PROFILING_ENABLE");
#ifdef CL_QUEUE_ON_DEVICE
        printFlag(CL_QUEUE_ON_DEVICE, "CL_QUEUE_ON_DEVICE");
#endif
#ifdef CL_QUEUE_ON_DEVICE_DEFAULT
        printFlag(CL_QUEUE_ON_DEVICE_DEFAULT, "CL_QUEUE_ON_DEVICE_DEFAULT");
#endif
        if (!printed) {
            std::cout << static_cast<cl_ulong>(value);
        }
        break;
    }
    case DeviceInfoValueType::SvmCapabilities: {
#ifdef CL_DEVICE_SVM_CAPABILITIES
        const auto value = readScalar<cl_device_svm_capabilities>(buffer);
        bool printed = false;
        auto printFlag = [&](cl_device_svm_capabilities flag, const char *name) {
            if (value & flag) {
                if (printed) {
                    std::cout << ' ';
                }
                std::cout << name;
                printed = true;
            }
        };
        printFlag(CL_DEVICE_SVM_COARSE_GRAIN_BUFFER, "CL_DEVICE_SVM_COARSE_GRAIN_BUFFER");
        printFlag(CL_DEVICE_SVM_FINE_GRAIN_BUFFER, "CL_DEVICE_SVM_FINE_GRAIN_BUFFER");
        printFlag(CL_DEVICE_SVM_FINE_GRAIN_SYSTEM, "CL_DEVICE_SVM_FINE_GRAIN_SYSTEM");
        printFlag(CL_DEVICE_SVM_ATOMICS, "CL_DEVICE_SVM_ATOMICS");
        if (!printed) {
            std::cout << static_cast<cl_ulong>(value);
        }
#else
        std::cout << "<unsupported>";
#endif
        break;
    }
    case DeviceInfoValueType::DeviceType: {
        const auto value = readScalar<cl_device_type>(buffer);
        bool printed = false;
        auto printFlag = [&](cl_device_type flag, const char *name) {
            if (value & flag) {
                if (printed) {
                    std::cout << ' ';
                }
                std::cout << name;
                printed = true;
            }
        };
        printFlag(CL_DEVICE_TYPE_DEFAULT, "CL_DEVICE_TYPE_DEFAULT");
        printFlag(CL_DEVICE_TYPE_CPU, "CL_DEVICE_TYPE_CPU");
        printFlag(CL_DEVICE_TYPE_GPU, "CL_DEVICE_TYPE_GPU");
        printFlag(CL_DEVICE_TYPE_ACCELERATOR, "CL_DEVICE_TYPE_ACCELERATOR");
#ifdef CL_DEVICE_TYPE_CUSTOM
        printFlag(CL_DEVICE_TYPE_CUSTOM, "CL_DEVICE_TYPE_CUSTOM");
#endif
        if (!printed) {
            std::cout << std::hex << std::showbase << static_cast<cl_ulong>(value) << std::dec << std::noshowbase;
        }
        break;
    }
    case DeviceInfoValueType::SizeTArray: {
        const auto values = readVector<size_t>(buffer);
        std::cout << '[';
        for (size_t i = 0; i < values.size(); i++) {
            if (i > 0) {
                std::cout << ' ';
            }
            std::cout << values[i];
        }
        std::cout << ']';
        break;
    }
    case DeviceInfoValueType::UintArray: {
        const auto values = readVector<cl_uint>(buffer);
        std::cout << '[';
        for (size_t i = 0; i < values.size(); i++) {
            if (i > 0) {
                std::cout << ' ';
            }
            std::cout << values[i];
        }
        std::cout << ']';
        break;
    }
    case DeviceInfoValueType::UlongArray: {
        const auto values = readVector<cl_ulong>(buffer);
        std::cout << '[';
        for (size_t i = 0; i < values.size(); i++) {
            if (i > 0) {
                std::cout << ' ';
            }
            std::cout << values[i];
        }
        std::cout << ']';
        break;
    }
    case DeviceInfoValueType::NameVersionArray: {
#ifdef CL_NAME_VERSION_MAX_NAME_SIZE
        const auto values = readVector<cl_name_version>(buffer);
        for (size_t i = 0; i < values.size(); i++) {
            if (i > 0) {
                std::cout << "; ";
            }
            std::cout << values[i].name << " (" << formatVersion(values[i].version) << ')';
        }
#endif
        break;
    }
#ifdef CL_NAME_VERSION_MAX_NAME_SIZE
    case DeviceInfoValueType::Version: {
        const auto value = readScalar<cl_version>(buffer);
        std::cout << formatVersion(value);
        break;
    }
#endif
    case DeviceInfoValueType::PlatformId: {
        const auto value = readScalar<cl_platform_id>(buffer);
        std::cout << value;
        break;
    }
    case DeviceInfoValueType::DeviceId: {
        const auto value = readScalar<cl_device_id>(buffer);
        std::cout << value;
        break;
    }
    }

    std::cout << '\n';
}

std::vector<DeviceInfoDescriptor> getDeviceInfoDescriptors() {
    std::vector<DeviceInfoDescriptor> descriptors{};
    descriptors.reserve(128);

    auto add = [&descriptors](cl_device_info param, const char *name, DeviceInfoValueType type) {
        descriptors.push_back(DeviceInfoDescriptor{param, name, type});
    };

    add(CL_DEVICE_TYPE, "CL_DEVICE_TYPE", DeviceInfoValueType::DeviceType);
    add(CL_DEVICE_VENDOR_ID, "CL_DEVICE_VENDOR_ID", DeviceInfoValueType::Uint);
    add(CL_DEVICE_MAX_COMPUTE_UNITS, "CL_DEVICE_MAX_COMPUTE_UNITS", DeviceInfoValueType::Uint);
    add(CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, "CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS", DeviceInfoValueType::Uint);
    add(CL_DEVICE_MAX_WORK_GROUP_SIZE, "CL_DEVICE_MAX_WORK_GROUP_SIZE", DeviceInfoValueType::SizeT);
    add(CL_DEVICE_MAX_WORK_ITEM_SIZES, "CL_DEVICE_MAX_WORK_ITEM_SIZES", DeviceInfoValueType::SizeTArray);
    add(CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR", DeviceInfoValueType::Uint);
    add(CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT", DeviceInfoValueType::Uint);
    add(CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT", DeviceInfoValueType::Uint);
    add(CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG", DeviceInfoValueType::Uint);
    add(CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT", DeviceInfoValueType::Uint);
    add(CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE", DeviceInfoValueType::Uint);
    add(CL_DEVICE_MAX_CLOCK_FREQUENCY, "CL_DEVICE_MAX_CLOCK_FREQUENCY", DeviceInfoValueType::Uint);
    add(CL_DEVICE_ADDRESS_BITS, "CL_DEVICE_ADDRESS_BITS", DeviceInfoValueType::Uint);
    add(CL_DEVICE_MAX_READ_IMAGE_ARGS, "CL_DEVICE_MAX_READ_IMAGE_ARGS", DeviceInfoValueType::Uint);
    add(CL_DEVICE_MAX_WRITE_IMAGE_ARGS, "CL_DEVICE_MAX_WRITE_IMAGE_ARGS", DeviceInfoValueType::Uint);
    add(CL_DEVICE_MAX_MEM_ALLOC_SIZE, "CL_DEVICE_MAX_MEM_ALLOC_SIZE", DeviceInfoValueType::Ulong);
    add(CL_DEVICE_IMAGE2D_MAX_WIDTH, "CL_DEVICE_IMAGE2D_MAX_WIDTH", DeviceInfoValueType::SizeT);
    add(CL_DEVICE_IMAGE2D_MAX_HEIGHT, "CL_DEVICE_IMAGE2D_MAX_HEIGHT", DeviceInfoValueType::SizeT);
    add(CL_DEVICE_IMAGE3D_MAX_WIDTH, "CL_DEVICE_IMAGE3D_MAX_WIDTH", DeviceInfoValueType::SizeT);
    add(CL_DEVICE_IMAGE3D_MAX_HEIGHT, "CL_DEVICE_IMAGE3D_MAX_HEIGHT", DeviceInfoValueType::SizeT);
    add(CL_DEVICE_IMAGE3D_MAX_DEPTH, "CL_DEVICE_IMAGE3D_MAX_DEPTH", DeviceInfoValueType::SizeT);
    add(CL_DEVICE_IMAGE_SUPPORT, "CL_DEVICE_IMAGE_SUPPORT", DeviceInfoValueType::Bool);
    add(CL_DEVICE_MAX_PARAMETER_SIZE, "CL_DEVICE_MAX_PARAMETER_SIZE", DeviceInfoValueType::SizeT);
    add(CL_DEVICE_MAX_SAMPLERS, "CL_DEVICE_MAX_SAMPLERS", DeviceInfoValueType::Uint);
    add(CL_DEVICE_MEM_BASE_ADDR_ALIGN, "CL_DEVICE_MEM_BASE_ADDR_ALIGN", DeviceInfoValueType::Uint);
    add(CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE, "CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE", DeviceInfoValueType::Uint);
    add(CL_DEVICE_SINGLE_FP_CONFIG, "CL_DEVICE_SINGLE_FP_CONFIG", DeviceInfoValueType::SingleFpConfig);
    add(CL_DEVICE_GLOBAL_MEM_CACHE_TYPE, "CL_DEVICE_GLOBAL_MEM_CACHE_TYPE", DeviceInfoValueType::CacheType);
    add(CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE, "CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE", DeviceInfoValueType::Uint);
    add(CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, "CL_DEVICE_GLOBAL_MEM_CACHE_SIZE", DeviceInfoValueType::Ulong);
    add(CL_DEVICE_GLOBAL_MEM_SIZE, "CL_DEVICE_GLOBAL_MEM_SIZE", DeviceInfoValueType::Ulong);
    add(CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE, "CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE", DeviceInfoValueType::Ulong);
    add(CL_DEVICE_MAX_CONSTANT_ARGS, "CL_DEVICE_MAX_CONSTANT_ARGS", DeviceInfoValueType::Uint);
    add(CL_DEVICE_LOCAL_MEM_TYPE, "CL_DEVICE_LOCAL_MEM_TYPE", DeviceInfoValueType::LocalMemType);
    add(CL_DEVICE_LOCAL_MEM_SIZE, "CL_DEVICE_LOCAL_MEM_SIZE", DeviceInfoValueType::Ulong);
    add(CL_DEVICE_ERROR_CORRECTION_SUPPORT, "CL_DEVICE_ERROR_CORRECTION_SUPPORT", DeviceInfoValueType::Bool);
    add(CL_DEVICE_PROFILING_TIMER_RESOLUTION, "CL_DEVICE_PROFILING_TIMER_RESOLUTION", DeviceInfoValueType::SizeT);
    add(CL_DEVICE_ENDIAN_LITTLE, "CL_DEVICE_ENDIAN_LITTLE", DeviceInfoValueType::Bool);
    add(CL_DEVICE_AVAILABLE, "CL_DEVICE_AVAILABLE", DeviceInfoValueType::Bool);
    add(CL_DEVICE_COMPILER_AVAILABLE, "CL_DEVICE_COMPILER_AVAILABLE", DeviceInfoValueType::Bool);
    add(CL_DEVICE_EXECUTION_CAPABILITIES, "CL_DEVICE_EXECUTION_CAPABILITIES", DeviceInfoValueType::ExecCapabilities);
    add(CL_DEVICE_QUEUE_PROPERTIES, "CL_DEVICE_QUEUE_PROPERTIES", DeviceInfoValueType::QueueProperties);
    add(CL_DEVICE_NAME, "CL_DEVICE_NAME", DeviceInfoValueType::String);
    add(CL_DEVICE_VENDOR, "CL_DEVICE_VENDOR", DeviceInfoValueType::String);
    add(CL_DRIVER_VERSION, "CL_DRIVER_VERSION", DeviceInfoValueType::String);
    add(CL_DEVICE_PROFILE, "CL_DEVICE_PROFILE", DeviceInfoValueType::String);
    add(CL_DEVICE_VERSION, "CL_DEVICE_VERSION", DeviceInfoValueType::String);
    add(CL_DEVICE_EXTENSIONS, "CL_DEVICE_EXTENSIONS", DeviceInfoValueType::String);
    add(CL_DEVICE_PLATFORM, "CL_DEVICE_PLATFORM", DeviceInfoValueType::PlatformId);

#ifdef CL_DEVICE_DOUBLE_FP_CONFIG
    add(CL_DEVICE_DOUBLE_FP_CONFIG, "CL_DEVICE_DOUBLE_FP_CONFIG", DeviceInfoValueType::DoubleFpConfig);
#endif
#ifdef CL_DEVICE_HALF_FP_CONFIG
    add(CL_DEVICE_HALF_FP_CONFIG, "CL_DEVICE_HALF_FP_CONFIG", DeviceInfoValueType::HalfFpConfig);
#endif
#ifdef CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF
    add(CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF", DeviceInfoValueType::Uint);
#endif
#ifdef CL_DEVICE_HOST_UNIFIED_MEMORY
    add(CL_DEVICE_HOST_UNIFIED_MEMORY, "CL_DEVICE_HOST_UNIFIED_MEMORY", DeviceInfoValueType::Bool);
#endif
    add(CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR, "CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR", DeviceInfoValueType::Uint);
    add(CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT, "CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT", DeviceInfoValueType::Uint);
    add(CL_DEVICE_NATIVE_VECTOR_WIDTH_INT, "CL_DEVICE_NATIVE_VECTOR_WIDTH_INT", DeviceInfoValueType::Uint);
    add(CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG, "CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG", DeviceInfoValueType::Uint);
    add(CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT, "CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT", DeviceInfoValueType::Uint);
    add(CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE, "CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE", DeviceInfoValueType::Uint);
#ifdef CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF
    add(CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF, "CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF", DeviceInfoValueType::Uint);
#endif
#ifdef CL_DEVICE_OPENCL_C_VERSION
    add(CL_DEVICE_OPENCL_C_VERSION, "CL_DEVICE_OPENCL_C_VERSION", DeviceInfoValueType::String);
#endif
#ifdef CL_DEVICE_LINKER_AVAILABLE
    add(CL_DEVICE_LINKER_AVAILABLE, "CL_DEVICE_LINKER_AVAILABLE", DeviceInfoValueType::Bool);
#endif
#ifdef CL_DEVICE_BUILT_IN_KERNELS
    add(CL_DEVICE_BUILT_IN_KERNELS, "CL_DEVICE_BUILT_IN_KERNELS", DeviceInfoValueType::String);
#endif
#ifdef CL_DEVICE_IMAGE_MAX_BUFFER_SIZE
    add(CL_DEVICE_IMAGE_MAX_BUFFER_SIZE, "CL_DEVICE_IMAGE_MAX_BUFFER_SIZE", DeviceInfoValueType::SizeT);
#endif
#ifdef CL_DEVICE_IMAGE_MAX_ARRAY_SIZE
    add(CL_DEVICE_IMAGE_MAX_ARRAY_SIZE, "CL_DEVICE_IMAGE_MAX_ARRAY_SIZE", DeviceInfoValueType::SizeT);
#endif
#ifdef CL_DEVICE_PARENT_DEVICE
    add(CL_DEVICE_PARENT_DEVICE, "CL_DEVICE_PARENT_DEVICE", DeviceInfoValueType::DeviceId);
#endif
#ifdef CL_DEVICE_PARTITION_MAX_SUB_DEVICES
    add(CL_DEVICE_PARTITION_MAX_SUB_DEVICES, "CL_DEVICE_PARTITION_MAX_SUB_DEVICES", DeviceInfoValueType::Uint);
#endif
#ifdef CL_DEVICE_PARTITION_PROPERTIES
    add(CL_DEVICE_PARTITION_PROPERTIES, "CL_DEVICE_PARTITION_PROPERTIES", DeviceInfoValueType::UlongArray);
#endif
#ifdef CL_DEVICE_PARTITION_AFFINITY_DOMAIN
    add(CL_DEVICE_PARTITION_AFFINITY_DOMAIN, "CL_DEVICE_PARTITION_AFFINITY_DOMAIN", DeviceInfoValueType::BitfieldHex);
#endif
#ifdef CL_DEVICE_PARTITION_TYPE
    add(CL_DEVICE_PARTITION_TYPE, "CL_DEVICE_PARTITION_TYPE", DeviceInfoValueType::UlongArray);
#endif
#ifdef CL_DEVICE_REFERENCE_COUNT
    add(CL_DEVICE_REFERENCE_COUNT, "CL_DEVICE_REFERENCE_COUNT", DeviceInfoValueType::Uint);
#endif
#ifdef CL_DEVICE_PREFERRED_INTEROP_USER_SYNC
    add(CL_DEVICE_PREFERRED_INTEROP_USER_SYNC, "CL_DEVICE_PREFERRED_INTEROP_USER_SYNC", DeviceInfoValueType::Bool);
#endif
#ifdef CL_DEVICE_PRINTF_BUFFER_SIZE
    add(CL_DEVICE_PRINTF_BUFFER_SIZE, "CL_DEVICE_PRINTF_BUFFER_SIZE", DeviceInfoValueType::SizeT);
#endif
#ifdef CL_DEVICE_IMAGE_PITCH_ALIGNMENT
    add(CL_DEVICE_IMAGE_PITCH_ALIGNMENT, "CL_DEVICE_IMAGE_PITCH_ALIGNMENT", DeviceInfoValueType::Uint);
#endif
#ifdef CL_DEVICE_IMAGE_BASE_ADDRESS_ALIGNMENT
    add(CL_DEVICE_IMAGE_BASE_ADDRESS_ALIGNMENT, "CL_DEVICE_IMAGE_BASE_ADDRESS_ALIGNMENT", DeviceInfoValueType::Uint);
#endif
#ifdef CL_DEVICE_MAX_READ_WRITE_IMAGE_ARGS
    add(CL_DEVICE_MAX_READ_WRITE_IMAGE_ARGS, "CL_DEVICE_MAX_READ_WRITE_IMAGE_ARGS", DeviceInfoValueType::Uint);
#endif
#ifdef CL_DEVICE_MAX_GLOBAL_VARIABLE_SIZE
    add(CL_DEVICE_MAX_GLOBAL_VARIABLE_SIZE, "CL_DEVICE_MAX_GLOBAL_VARIABLE_SIZE", DeviceInfoValueType::SizeT);
#endif
#ifdef CL_DEVICE_QUEUE_ON_DEVICE_PROPERTIES
    add(CL_DEVICE_QUEUE_ON_DEVICE_PROPERTIES, "CL_DEVICE_QUEUE_ON_DEVICE_PROPERTIES", DeviceInfoValueType::BitfieldHex);
    add(CL_DEVICE_QUEUE_ON_DEVICE_PREFERRED_SIZE, "CL_DEVICE_QUEUE_ON_DEVICE_PREFERRED_SIZE", DeviceInfoValueType::Uint);
    add(CL_DEVICE_QUEUE_ON_DEVICE_MAX_SIZE, "CL_DEVICE_QUEUE_ON_DEVICE_MAX_SIZE", DeviceInfoValueType::Uint);
#endif
#ifdef CL_DEVICE_MAX_ON_DEVICE_QUEUES
    add(CL_DEVICE_MAX_ON_DEVICE_QUEUES, "CL_DEVICE_MAX_ON_DEVICE_QUEUES", DeviceInfoValueType::Uint);
#endif
#ifdef CL_DEVICE_MAX_ON_DEVICE_EVENTS
    add(CL_DEVICE_MAX_ON_DEVICE_EVENTS, "CL_DEVICE_MAX_ON_DEVICE_EVENTS", DeviceInfoValueType::Uint);
#endif
#ifdef CL_DEVICE_SVM_CAPABILITIES
    add(CL_DEVICE_SVM_CAPABILITIES, "CL_DEVICE_SVM_CAPABILITIES", DeviceInfoValueType::SvmCapabilities);
#endif
#ifdef CL_DEVICE_GLOBAL_VARIABLE_PREFERRED_TOTAL_SIZE
    add(CL_DEVICE_GLOBAL_VARIABLE_PREFERRED_TOTAL_SIZE, "CL_DEVICE_GLOBAL_VARIABLE_PREFERRED_TOTAL_SIZE", DeviceInfoValueType::SizeT);
#endif
#ifdef CL_DEVICE_MAX_PIPE_ARGS
    add(CL_DEVICE_MAX_PIPE_ARGS, "CL_DEVICE_MAX_PIPE_ARGS", DeviceInfoValueType::Uint);
#endif
#ifdef CL_DEVICE_PIPE_MAX_ACTIVE_RESERVATIONS
    add(CL_DEVICE_PIPE_MAX_ACTIVE_RESERVATIONS, "CL_DEVICE_PIPE_MAX_ACTIVE_RESERVATIONS", DeviceInfoValueType::Uint);
#endif
#ifdef CL_DEVICE_PIPE_MAX_PACKET_SIZE
    add(CL_DEVICE_PIPE_MAX_PACKET_SIZE, "CL_DEVICE_PIPE_MAX_PACKET_SIZE", DeviceInfoValueType::Uint);
#endif
#ifdef CL_DEVICE_PREFERRED_PLATFORM_ATOMIC_ALIGNMENT
    add(CL_DEVICE_PREFERRED_PLATFORM_ATOMIC_ALIGNMENT, "CL_DEVICE_PREFERRED_PLATFORM_ATOMIC_ALIGNMENT", DeviceInfoValueType::Uint);
#endif
#ifdef CL_DEVICE_PREFERRED_GLOBAL_ATOMIC_ALIGNMENT
    add(CL_DEVICE_PREFERRED_GLOBAL_ATOMIC_ALIGNMENT, "CL_DEVICE_PREFERRED_GLOBAL_ATOMIC_ALIGNMENT", DeviceInfoValueType::Uint);
#endif
#ifdef CL_DEVICE_PREFERRED_LOCAL_ATOMIC_ALIGNMENT
    add(CL_DEVICE_PREFERRED_LOCAL_ATOMIC_ALIGNMENT, "CL_DEVICE_PREFERRED_LOCAL_ATOMIC_ALIGNMENT", DeviceInfoValueType::Uint);
#endif
#ifdef CL_DEVICE_IL_VERSION
    add(CL_DEVICE_IL_VERSION, "CL_DEVICE_IL_VERSION", DeviceInfoValueType::String);
#endif
#ifdef CL_DEVICE_MAX_NUM_SUB_GROUPS
    add(CL_DEVICE_MAX_NUM_SUB_GROUPS, "CL_DEVICE_MAX_NUM_SUB_GROUPS", DeviceInfoValueType::Uint);
#endif
#ifdef CL_DEVICE_SUB_GROUP_INDEPENDENT_FORWARD_PROGRESS
    add(CL_DEVICE_SUB_GROUP_INDEPENDENT_FORWARD_PROGRESS, "CL_DEVICE_SUB_GROUP_INDEPENDENT_FORWARD_PROGRESS", DeviceInfoValueType::Bool);
#endif
#ifdef CL_DEVICE_NUMERIC_VERSION
    add(CL_DEVICE_NUMERIC_VERSION, "CL_DEVICE_NUMERIC_VERSION", DeviceInfoValueType::Version);
#endif
#ifdef CL_DEVICE_EXTENSIONS_WITH_VERSION
    add(CL_DEVICE_EXTENSIONS_WITH_VERSION, "CL_DEVICE_EXTENSIONS_WITH_VERSION", DeviceInfoValueType::NameVersionArray);
#endif
#ifdef CL_DEVICE_ILS_WITH_VERSION
    add(CL_DEVICE_ILS_WITH_VERSION, "CL_DEVICE_ILS_WITH_VERSION", DeviceInfoValueType::NameVersionArray);
#endif
#ifdef CL_DEVICE_BUILT_IN_KERNELS_WITH_VERSION
    add(CL_DEVICE_BUILT_IN_KERNELS_WITH_VERSION, "CL_DEVICE_BUILT_IN_KERNELS_WITH_VERSION", DeviceInfoValueType::NameVersionArray);
#endif
#ifdef CL_DEVICE_ATOMIC_MEMORY_CAPABILITIES
    add(CL_DEVICE_ATOMIC_MEMORY_CAPABILITIES, "CL_DEVICE_ATOMIC_MEMORY_CAPABILITIES", DeviceInfoValueType::BitfieldHex);
#endif
#ifdef CL_DEVICE_ATOMIC_FENCE_CAPABILITIES
    add(CL_DEVICE_ATOMIC_FENCE_CAPABILITIES, "CL_DEVICE_ATOMIC_FENCE_CAPABILITIES", DeviceInfoValueType::BitfieldHex);
#endif
#ifdef CL_DEVICE_NON_UNIFORM_WORK_GROUP_SUPPORT
    add(CL_DEVICE_NON_UNIFORM_WORK_GROUP_SUPPORT, "CL_DEVICE_NON_UNIFORM_WORK_GROUP_SUPPORT", DeviceInfoValueType::Bool);
#endif
#ifdef CL_DEVICE_OPENCL_C_ALL_VERSIONS
    add(CL_DEVICE_OPENCL_C_ALL_VERSIONS, "CL_DEVICE_OPENCL_C_ALL_VERSIONS", DeviceInfoValueType::NameVersionArray);
#endif
#ifdef CL_DEVICE_PREFERRED_WORK_GROUP_SIZE_MULTIPLE
    add(CL_DEVICE_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, "CL_DEVICE_PREFERRED_WORK_GROUP_SIZE_MULTIPLE", DeviceInfoValueType::SizeT);
#endif
#ifdef CL_DEVICE_WORK_GROUP_COLLECTIVE_FUNCTIONS_SUPPORT
    add(CL_DEVICE_WORK_GROUP_COLLECTIVE_FUNCTIONS_SUPPORT, "CL_DEVICE_WORK_GROUP_COLLECTIVE_FUNCTIONS_SUPPORT", DeviceInfoValueType::Bool);
#endif
#ifdef CL_DEVICE_GENERIC_ADDRESS_SPACE_SUPPORT
    add(CL_DEVICE_GENERIC_ADDRESS_SPACE_SUPPORT, "CL_DEVICE_GENERIC_ADDRESS_SPACE_SUPPORT", DeviceInfoValueType::Bool);
#endif
#ifdef CL_DEVICE_DEVICE_ENQUEUE_CAPABILITIES
    add(CL_DEVICE_DEVICE_ENQUEUE_CAPABILITIES, "CL_DEVICE_DEVICE_ENQUEUE_CAPABILITIES", DeviceInfoValueType::BitfieldHex);
#endif
#ifdef CL_DEVICE_PIPE_SUPPORT
    add(CL_DEVICE_PIPE_SUPPORT, "CL_DEVICE_PIPE_SUPPORT", DeviceInfoValueType::Bool);
#endif

    return descriptors;
}

void printAllDeviceInfo(size_t indentLevel, cl_device_id device) {
    const std::string indent(indentLevel, '\t');
    std::cout << indent << "Device info:" << '\n';

    const auto descriptors = getDeviceInfoDescriptors();
    for (const auto &desc : descriptors) {
        printDeviceInfoValue(indent + "\t", device, desc);
    }
}

// ------------------------------------------------------------------------- Helpers for creating/freeing platforms and devices

std::vector<cl_platform_id> getPlatforms() {
    cl_uint numPlatforms;
    cl_int retVal = clGetPlatformIDs(0, nullptr, &numPlatforms);
    if (retVal != CL_SUCCESS) {
        std::cout << "OpenCL platforms: NONE (clGetPlatformIDs returned " << oclErrorToString(retVal) << ")\n";
        return {};
    }
    std::vector<cl_platform_id> platforms{numPlatforms};
    CL_SUCCESS_OR_ERROR(clGetPlatformIDs(numPlatforms, platforms.data(), nullptr), "Querying platforms");
    return platforms;
}

std::vector<cl_device_id> getDevices(cl_platform_id platform) {
    cl_uint numDevices;
    CL_SUCCESS_OR_ERROR(clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, nullptr, &numDevices), "Querying devices");
    if (numDevices == 0) {
        return {};
    }

    std::vector<cl_device_id> devices{numDevices};
    CL_SUCCESS_OR_ERROR(clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, numDevices, devices.data(), nullptr), "Querying devices");
    return devices;
}

std::vector<cl_device_id> createSubDevices(cl_device_id rootDevice) {
    cl_device_affinity_domain domain{};
    CL_SUCCESS_OR_ERROR(clGetDeviceInfo(rootDevice, CL_DEVICE_PARTITION_AFFINITY_DOMAIN, sizeof(domain), &domain, NULL), "Querying subdevices");
    if ((domain & CL_DEVICE_AFFINITY_DOMAIN_NEXT_PARTITIONABLE) == 0) {
        return {};
    }
    if ((domain & CL_DEVICE_AFFINITY_DOMAIN_NUMA) == 0) {
        return {};
    }

    const cl_device_partition_property properties[] = {CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN, CL_DEVICE_AFFINITY_DOMAIN_NUMA, 0};
    cl_uint numSubDevices{};
    CL_SUCCESS_OR_ERROR(clCreateSubDevices(rootDevice, properties, 0, nullptr, &numSubDevices), "Creating subdevices");

    std::vector<cl_device_id> result = {};
    result.resize(numSubDevices);
    CL_SUCCESS_OR_ERROR(clCreateSubDevices(rootDevice, properties, numSubDevices, result.data(), nullptr), "Creating subdevices");
    return result;
}

void freeSubDevices(std::vector<cl_device_id> &subDevices) {
    for (cl_device_id subDevice : subDevices) {
        CL_SUCCESS_OR_ERROR(clReleaseDevice(subDevice), "Releasing subdevice");
    }
}

// ------------------------------------------------------------------------- Printing functions

void showPlatform(size_t indentLevel, cl_platform_id platform, size_t platformIndex) {
    const std::string indent0(indentLevel + 0, '\t');
    char bufferString[4096];

    CL_SUCCESS_OR_ERROR(clGetPlatformInfo(platform, CL_PLATFORM_NAME, sizeof(bufferString), bufferString, nullptr), "Querying platform name");
    std::cout << indent0 << "Platform " << platformIndex << ": " << bufferString << '\n';
}

void showDevice(size_t indentLevel, cl_device_id device, const std::string &deviceLabel, size_t deviceIndex) {
    const std::string indent0(indentLevel + 0, '\t');
    const std::string indent1(indentLevel + 1, '\t');
    const std::string indent2(indentLevel + 2, '\t');
    char bufferString[4096];

    // Print device header
    CL_SUCCESS_OR_ERROR(clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(bufferString), bufferString, nullptr), "Querying device name");
    std::cout << indent0 << deviceLabel << " " << deviceIndex << ": " << bufferString;
    if (const IntelProduct intelProduct = getIntelProduct(device); intelProduct != IntelProduct::Unknown) {
        std::cout << " (" << std::to_string(intelProduct) << ")";
    }
    std::cout << '\n';

    cl_device_svm_capabilities svmCaps{};
    clGetDeviceInfo(device, CL_DEVICE_SVM_CAPABILITIES, sizeof(svmCaps), &svmCaps, nullptr);
    std::cout << indent1 << "SVM capabilities: ";
    if (svmCaps & CL_DEVICE_SVM_COARSE_GRAIN_BUFFER) {
        std::cout << "CL_DEVICE_SVM_COARSE_GRAIN_BUFFER ";
    }
    if (svmCaps & CL_DEVICE_SVM_FINE_GRAIN_BUFFER) {
        std::cout << "CL_DEVICE_SVM_FINE_GRAIN_BUFFER ";
    }
    if (svmCaps & CL_DEVICE_SVM_FINE_GRAIN_SYSTEM) {
        std::cout << "CL_DEVICE_SVM_FINE_GRAIN_SYSTEM ";
    }
    if (svmCaps & CL_DEVICE_SVM_ATOMICS) {
        std::cout << "CL_DEVICE_SVM_ATOMICS ";
    }
    std::cout << '\n';

    printAllDeviceInfo(indentLevel + 1, device);

    // Print queue families support
    const bool queueuFamiliesSupported = ExtensionsHelper{device}.isCommandQueueFamiliesSupported();
    if (!queueuFamiliesSupported) {
        std::cout << indent1 << "Extension cl_intel_command_queue_families is NOT SUPPORTED\n";
        return;
    }

    // Print default queue
    size_t defaultFamilyIndex{};
    size_t defaultQueueIndex{};
    {
        cl_int retVal{};
        cl_context context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &retVal);
        CL_SUCCESS_OR_ERROR(retVal, "Context creation");
        cl_command_queue defaultQueue = clCreateCommandQueueWithProperties(context, device, nullptr, &retVal);
        CL_SUCCESS_OR_ERROR(retVal, "Queue creation");

        CL_SUCCESS_OR_ERROR(clGetCommandQueueInfo(defaultQueue, CL_QUEUE_FAMILY_INTEL, sizeof(defaultFamilyIndex), &defaultFamilyIndex, nullptr), "Querying family index");
        CL_SUCCESS_OR_ERROR(clGetCommandQueueInfo(defaultQueue, CL_QUEUE_INDEX_INTEL, sizeof(defaultQueueIndex), &defaultQueueIndex, nullptr), "Querying queue index");

        CL_SUCCESS_OR_ERROR(clReleaseCommandQueue(defaultQueue), "Releasing queue");
        CL_SUCCESS_OR_ERROR(clReleaseContext(context), "Releasing context");
    }
    std::cout << indent1 << "defaultFamilyIndex=" << defaultFamilyIndex << " defaultQueueIndex=" << defaultQueueIndex << '\n';

    // Print queue families
    size_t familyPropertiesSize{};
    CL_SUCCESS_OR_ERROR(clGetDeviceInfo(device, CL_DEVICE_QUEUE_FAMILY_PROPERTIES_INTEL, 0, nullptr, &familyPropertiesSize), "Querying families");
    const size_t numQueueFamilies = familyPropertiesSize / sizeof(cl_queue_family_properties_intel);
    auto queueFamilies = std::make_unique<cl_queue_family_properties_intel[]>(numQueueFamilies);
    CL_SUCCESS_OR_ERROR(clGetDeviceInfo(device, CL_DEVICE_QUEUE_FAMILY_PROPERTIES_INTEL, familyPropertiesSize, queueFamilies.get(), nullptr), "Querying families");
    std::cout << indent1 << "Queue families:\n";
    for (size_t familyIndex = 0u; familyIndex < numQueueFamilies; familyIndex++) {
        const cl_queue_family_properties_intel &queueFamily = queueFamilies[familyIndex];
        std::cout << indent2 << queueFamily.count << ' ' << queueFamily.name;
        if (familyIndex == defaultFamilyIndex) {
            std::cout << " (default)";
        }
        std::cout << '\n';
    }
}

void showDeviceAndItsSubDevices(size_t indentLevel, size_t deviceLevel, cl_device_id device, size_t deviceIndex) {
    const std::string deviceNames[] = {
        "RootDevice",
        "SubDevice",
        "SubSubDevice",
    };
    const std::string deviceLabel = deviceNames[deviceLevel];

    showDevice(indentLevel, device, deviceLabel, deviceIndex);

    std::vector<cl_device_id> subDevices = createSubDevices(device);
    for (auto subDeviceIndex = 0u; subDeviceIndex < subDevices.size(); subDeviceIndex++) {
        showDeviceAndItsSubDevices(indentLevel + 1, deviceLevel + 1, subDevices[subDeviceIndex], subDeviceIndex);
    }
    freeSubDevices(subDevices);
}

// ------------------------------------------------------------------------- Main procedure

int main() {
    Configuration::loadDefaultConfiguration();

    std::vector<cl_platform_id> platforms = getPlatforms();
    for (auto platformIndex = 0u; platformIndex < platforms.size(); platformIndex++) {
        cl_platform_id platform = platforms[platformIndex];
        showPlatform(0u, platform, platformIndex);

        std::vector<cl_device_id> devices = getDevices(platform);
        for (auto deviceIndex = 0u; deviceIndex < devices.size(); deviceIndex++) {
            cl_device_id device = devices[deviceIndex];
            showDeviceAndItsSubDevices(1u, 0u, device, deviceIndex);
        }
    }

    return 0;
}
