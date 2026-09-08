/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "print_device_info.h"

#include "error.h"
#include "offload_print.h"
#include "ol.h"

#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

template <typename Type>
struct Expected {
    Type value{};
    ol_result_t error = OL_SUCCESS;

    explicit operator bool() const {
        return error == OL_SUCCESS;
    }
};

std::string formatDeviceInfoError(ol_result_t error) {
    if (error == OL_SUCCESS) {
        return "OL_SUCCESS";
    }
    if (error->Details && error->Details[0] != '\0') {
        return error->Details;
    }

    return std::string(OL::toString(error->Code));
}

template <typename>
inline constexpr bool UnsupportedDeviceInfoFieldType = false;

template <typename Type>
Expected<Type> getDeviceInfoField(ol_device_handle_t device,
                                  ol_device_info_t info) {
    if constexpr (std::is_same_v<Type, std::string>) {
        size_t size = 0;
        if (const auto error = olGetDeviceInfoSize(device, info, &size);
            error != OL_SUCCESS) {
            return {{}, error};
        }

        std::string value(size, '\0');
        if (size != 0) {
            if (const auto error =
                    olGetDeviceInfo(device, info, size, value.data());
                error != OL_SUCCESS) {
                return {{}, error};
            }
            if (value.back() == '\0') {
                value.pop_back();
            }
        }
        return {std::move(value), OL_SUCCESS};
    } else {
        Type value{};
        if (const auto error =
                olGetDeviceInfo(device, info, sizeof(value), &value);
            error != OL_SUCCESS) {
            return {{}, error};
        }
        return {value, OL_SUCCESS};
    }
}

template <typename Type>
std::string formatDeviceInfoField(const Expected<Type> &field) {
    if (!field) {
        return formatDeviceInfoError(field.error);
    }

    if constexpr (std::is_same_v<Type, bool>) {
        return field.value ? "true" : "false";
    } else if constexpr (std::is_integral_v<Type>) {
        using UnsignedType = std::make_unsigned_t<Type>;
        std::ostringstream output;
        output << std::dec << +field.value << " (" << std::hex
               << +static_cast<UnsignedType>(field.value) << ")";
        return output.str();
    } else if constexpr (std::is_same_v<Type, std::string>) {
        return field.value;
    } else if constexpr (std::is_same_v<Type, ol_dimensions_t>) {
        std::ostringstream output;
        output << "(" << field.value.x << ", " << field.value.y << ", "
               << field.value.z << ")";
        return output.str();
    } else if constexpr (std::is_enum_v<Type>) {
        return std::string(OL::toString(field.value));
    } else {
        static_assert(UnsupportedDeviceInfoFieldType<Type>,
                      "Unsupported device info field type");
    }
}

template <typename Type>
Expected<Type> getPlatformInfoField(ol_platform_handle_t platform,
                                    ol_platform_info_t info) {
    if constexpr (std::is_same_v<Type, std::string>) {
        size_t size = 0;
        if (const auto error = olGetPlatformInfoSize(platform, info, &size);
            error != OL_SUCCESS) {
            return {{}, error};
        }

        std::string value(size, '\0');
        if (size != 0) {
            if (const auto error =
                    olGetPlatformInfo(platform, info, size, value.data());
                error != OL_SUCCESS) {
                return {{}, error};
            }
            if (value.back() == '\0') {
                value.pop_back();
            }
        }
        return {std::move(value), OL_SUCCESS};
    } else {
        Type value{};
        if (const auto error =
                olGetPlatformInfo(platform, info, sizeof(value), &value);
            error != OL_SUCCESS) {
            return {{}, error};
        }
        return {value, OL_SUCCESS};
    }
}

std::string makeFieldName(std::string_view enumerator,
                          std::string_view prefix,
                          std::string_view fieldGroup = {}) {
    if (enumerator.substr(0, prefix.size()) == prefix) {
        enumerator.remove_prefix(prefix.size());
    }

    std::string result(fieldGroup);
    bool capitalize = true;
    for (const char character : enumerator) {
        if (character == '_') {
            result.push_back(' ');
            capitalize = true;
            continue;
        }

        const auto unsignedCharacter = static_cast<unsigned char>(character);
        result.push_back(static_cast<char>(
            capitalize ? std::toupper(unsignedCharacter)
                       : std::tolower(unsignedCharacter)));
        capitalize = false;
    }
    return result;
}

template <typename Type>
void addDeviceField(std::vector<OL::DeviceInfoField> &fields,
                    ol_device_handle_t device, ol_device_info_t info,
                    bool showInDeviceListItem) {
    fields.push_back(
        {makeFieldName(OL::toString(info), "OL_DEVICE_INFO_"),
         formatDeviceInfoField(getDeviceInfoField<Type>(device, info)),
         showInDeviceListItem});
}

template <typename Type>
void addPlatformField(std::vector<OL::DeviceInfoField> &fields,
                      const Expected<ol_platform_handle_t> &platform,
                      ol_platform_info_t info, bool showInDeviceListItem) {
    Expected<Type> value;
    if (platform) {
        value = getPlatformInfoField<Type>(platform.value, info);
    } else {
        value.error = platform.error;
    }

    fields.push_back(
        {makeFieldName(OL::toString(info), "OL_PLATFORM_INFO_", "Platform "),
         formatDeviceInfoField(value), showInDeviceListItem});
}

struct AvailableDevices {
    std::vector<ol_device_handle_t> devices;
};

bool collectDevice(ol_device_handle_t device, void *userData) {
    static_cast<AvailableDevices *>(userData)->devices.push_back(device);
    return true;
}
} // namespace

namespace OL {

std::vector<DeviceInfoField> getDeviceInfo(ol_device_handle_t device) {
    std::vector<DeviceInfoField> fields;
    fields.reserve(13);

    const auto platform = ::getDeviceInfoField<ol_platform_handle_t>(
        device, OL_DEVICE_INFO_PLATFORM);

    addPlatformField<ol_platform_backend_t>(
        fields, platform, OL_PLATFORM_INFO_BACKEND, true);
    addPlatformField<std::string>(fields, platform, OL_PLATFORM_INFO_NAME,
                                  true);
    addPlatformField<std::string>(fields, platform, OL_PLATFORM_INFO_VERSION,
                                  false);
    addDeviceField<std::string>(fields, device, OL_DEVICE_INFO_NAME, true);
    addDeviceField<ol_device_type_t>(fields, device, OL_DEVICE_INFO_TYPE, true);
    addDeviceField<std::string>(fields, device, OL_DEVICE_INFO_VENDOR, false);
    addDeviceField<std::string>(fields, device, OL_DEVICE_INFO_DRIVER_VERSION,
                                false);
    addDeviceField<uint32_t>(fields, device, OL_DEVICE_INFO_DRIVER_ID, false);
    addDeviceField<uint32_t>(fields, device, OL_DEVICE_INFO_VENDOR_ID, false);
    addDeviceField<uint32_t>(fields, device,
                             OL_DEVICE_INFO_MAX_CLOCK_FREQUENCY, false);
    addDeviceField<uint32_t>(fields, device, OL_DEVICE_INFO_NUM_COMPUTE_UNITS,
                             false);
    addDeviceField<uint32_t>(fields, device,
                             OL_DEVICE_INFO_MAX_WORK_GROUP_SIZE, false);
    addDeviceField<uint64_t>(fields, device, OL_DEVICE_INFO_GLOBAL_MEM_SIZE,
                             false);

    return fields;
}

void printDeviceInfo(std::ostream &output) {
    OlState ol;

    for (const auto &field : getDeviceInfo(ol.device)) {
        output << field.key << ": " << field.value << std::endl;
    }
    output << std::endl;
}

void printAvailableDevices() {
    OlInitGuard olInitGuard(nullptr);

    AvailableDevices availableDevices;
    OL_RESULT_SUCCESS_OR_ERROR(
        olIterateDevices(collectDevice, &availableDevices));

    std::cout << "Liboffload devices: " << availableDevices.devices.size()
              << std::endl;
    for (size_t index = 0; index < availableDevices.devices.size(); ++index) {
        const auto device = availableDevices.devices[index];
        std::cout << "  Device " << index << ":" << std::endl;
        for (const auto &field : getDeviceInfo(device)) {
            if (field.showInDeviceListItem) {
                std::cout << "    " << field.key << ": " << field.value
                          << std::endl;
            }
        }
        std::cout << "    select this device with --olDeviceIndex=" << index
                  << std::endl;
    }
    std::cout << std::endl;
}

} // namespace OL
