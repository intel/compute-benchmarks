/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "common_help_message.h"

#include <sstream>

std::string CommonHelpMessage::errorIgnoredCommandLineArgs() {
    return "The following command line arguments were ignored: ";
}

std::string CommonHelpMessage::errorUnsetArguments() {
    return "The following test case arguments were not set to any value: ";
}

std::string CommonHelpMessage::compression(const char *target) {
    std::ostringstream result{};
    result << "Select if the "
           << target
           << " is to be compressed. Will be skipped, if device does not support compression";
    return result.str();
}

std::string CommonHelpMessage::forceBlitter() {
    return "Force blitter engine. Test will be skipped if device does not support blitter. "
           "Warning: in OpenCL blitter may still be used even if not forced";
}

std::string CommonHelpMessage::useEvents() {
    return "Perform GPU-side measurements using events";
}
std::string CommonHelpMessage::measuredCommandsCount() {
    return "Number of commands being measured. Result is later divided by this number, to achieve time of a single command";
}

std::string CommonHelpMessage::atomicDataType() {
    return "Data type of the atomic. Keep in mind not all operations are supported for floating points";
}

std::string CommonHelpMessage::hostptrBufferReuse() {
    return "How hostptr allocation can be reused due to previous operations";
}

std::string CommonHelpMessage::writeOperation() {
    return "Which operation is used, whether write or read";
}
