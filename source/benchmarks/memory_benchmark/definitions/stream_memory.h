/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/compression_argument.h"
#include "framework/argument/enum/buffer_contents_argument.h"
#include "framework/argument/enum/stream_memory_type_argument.h"
#include "framework/argument/enum/usm_runtime_memory_placement_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/error.h"
#include "framework/utility/stream_memory_helper.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <string>

struct StreamMemoryArguments : TestCaseArgumentContainer {
    StreamMemoryTypeArgument type;
    ByteSizeArgument size;
    BooleanArgument useEvents;
    BufferContentsArgument contents;
    UsmRuntimeMemoryPlacementArgument memoryPlacement;
    PositiveIntegerArgument partialMultiplier;
    PositiveIntegerArgument vectorSize;
    PositiveIntegerArgument lws;

    StreamMemoryArguments()
        : type(*this, "type", "Memory streaming type"),
          size(*this, "size", "Size of the memory to stream. Rounded down to a multiple of datatype size and work-group size."),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()),
          contents(*this, "contents", "Buffer contents zeros/random"),
          memoryPlacement(*this, "memory", "Memory type used for stream"),
          partialMultiplier(*this, "multiplier", "multiplies id used for accessing the resources to simulate partials"),
          vectorSize(*this, "vectorSize", "size of uint vector type 1/2/4/8/16"),
          lws(*this, "lws", "local work size") {}
};

struct StreamMemory : TestCase<StreamMemoryArguments> {
    using TestCase<StreamMemoryArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "StreamMemory";
    }

    std::string getHelp() const override {
        return "Streams memory inside of kernel in a fashion described by 'type'. Copy means one "
               "memory location is read from and the second one is written to. Triad means two "
               "buffers are read and one is written to. In read and write memory is only read or "
               "written to. With --verify, Read also stores one element per 256 work groups (--verificationMode=light) "
               "or every element (--verificationMode=complete) so the output can be checked.";
    }
};

inline constexpr uint32_t streamMemoryScalarValue = 2u;

inline std::array<uint32_t, 16> getStreamMemoryScalarValue() {
    std::array<uint32_t, 16> value{};
    value.fill(streamMemoryScalarValue);
    return value;
}

inline bool isStreamMemoryUnsupported(const StreamMemoryArguments &arguments) {
    if (arguments.partialMultiplier <= 1u) {
        return false;
    }
    return arguments.type == StreamMemoryType::Scale ||
           arguments.type == StreamMemoryType::Triad ||
           (arguments.type == StreamMemoryType::Write && arguments.contents != BufferContents::Zeros);
}

struct StreamMemoryDispatch {
    size_t elementSize;
    size_t groupSize;
    size_t groupCount;
    size_t globalSize;
    // Requested size rounded down to a whole number of work groups.
    size_t bufferSize;
};

inline TestResult getStreamMemoryDispatch(const StreamMemoryArguments &arguments, StreamMemoryDispatch &dispatch) {
    dispatch.elementSize = arguments.vectorSize * sizeof(uint32_t);
    const auto workSize = StreamMemoryHelper::getWorkSize(arguments.size, dispatch.elementSize, arguments.lws);
    if (!workSize) {
        std::cerr << "size " << static_cast<size_t>(arguments.size) << " is smaller than one "
                  << dispatch.elementSize << "-byte element" << std::endl;
        return TestResult::InvalidArgs;
    }

    dispatch.groupSize = workSize->groupSize;
    dispatch.groupCount = workSize->globalSize / workSize->groupSize;
    dispatch.globalSize = workSize->globalSize;
    dispatch.bufferSize = workSize->bufferSize;

    if (!StreamMemoryHelper::fitsKernelIndexRange(dispatch.globalSize, arguments.partialMultiplier)) {
        std::cerr << "size " << static_cast<size_t>(arguments.size) << " with vectorSize " << static_cast<size_t>(arguments.vectorSize)
                  << " and multiplier " << static_cast<size_t>(arguments.partialMultiplier)
                  << " needs index " << dispatch.globalSize * arguments.partialMultiplier << ", which exceeds the 32-bit index range of the stream kernels" << std::endl;
        return TestResult::InvalidArgs;
    }
    return TestResult::Success;
}

inline size_t getStreamMemoryTransferSize(const StreamMemoryArguments &arguments, size_t bufferSize) {
    size_t transferSize = bufferSize;
    if (arguments.type == StreamMemoryType::Scale) {
        transferSize *= 2;
    } else if (arguments.type == StreamMemoryType::Triad) {
        transferSize *= 3;
    }
    if (arguments.partialMultiplier > 1u) {
        transferSize /= arguments.partialMultiplier;
    }
    return transferSize;
}

inline const char *getStreamMemoryKernelName(const StreamMemoryArguments &arguments, StreamMemoryHelper::Verification verification) {
    switch (arguments.type) {
    case StreamMemoryType::Read:
        switch (verification) {
        case StreamMemoryHelper::Verification::Light:
            return "readWithMultiplierSample";
        case StreamMemoryHelper::Verification::Complete:
            return "readWithMultiplierStore";
        default:
            return "readWithMultiplier";
        }
    case StreamMemoryType::Write:
        return arguments.contents == BufferContents::Random ? "write_random" : "writeWithMultiplier";
    case StreamMemoryType::Scale:
        return "scale";
    case StreamMemoryType::Triad:
        return "triad";
    case StreamMemoryType::Unknown:
        break;
    }
    FATAL_ERROR("Unknown StreamMemoryType");
}

// write_random has the value to write embedded in the kernel code.
inline bool streamMemoryHasScalarArgument(const StreamMemoryArguments &arguments) {
    return arguments.type != StreamMemoryType::Write || arguments.contents != BufferContents::Random;
}

inline std::string getStreamMemoryTypeDefinition(size_t vectorSize) {
    std::string streamType = "uint";
    if (vectorSize > 1) {
        streamType += std::to_string(vectorSize);
    }
    return streamType;
}
