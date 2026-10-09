/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/sycl/sycl.h"
#include "framework/sycl/utility/buffer_contents_helper_sycl.h"
#include "framework/sycl/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/stream_memory_helper.h"
#include "framework/utility/timer.h"

#include "definitions/stream_memory.h"

#include <algorithm>
#include <optional>
#include <vector>

using namespace SYCL;

// The kernels below must stay in sync with source/kernels/memory_benchmark_stream_memory.cl,
// which the L0 and OpenCL implementations run.

namespace {

// STREAM_TYPE of the .cl kernel. A native vector, not sycl::vec, because sycl::vec cannot be read
// through the volatile pointer runRead() uses.
template <int vectorSize>
using StreamTypeT = uint32_t __attribute__((vector_size(vectorSize * sizeof(uint32_t))));

inline bool multipliedIndex(const sycl::nd_item<1> &item, uint32_t multiplier, uint32_t &index) {
    index = static_cast<uint32_t>(item.get_global_id(0));
    if (multiplier > 1) {
        index *= multiplier;
        return index < item.get_global_range(0);
    }
    return true;
}

template <int vectorSize>
class StreamMemoryBenchmark {
  public:
    using StreamT = StreamTypeT<vectorSize>;

    StreamMemoryBenchmark(Sycl &sycl, const StreamMemoryArguments &arguments, const StreamMemoryDispatch &dispatch)
        : sycl(sycl), arguments(arguments), verifyEnabled(Configuration::get().verify), bufferSize(dispatch.bufferSize),
          ndRange(sycl::range<1>{dispatch.globalSize}, sycl::range<1>{dispatch.groupSize}) {
        for (int lane = 0; lane < vectorSize; lane++) {
            scalarValue[lane] = streamMemoryScalarValue;
        }
    }

    TestResult initBuffers() {
        for (size_t i = 0; i < bufferCount(); i++) {
            void *buffer = UsmHelper::allocate(arguments.memoryPlacement, sycl, bufferSize);
            if (buffer == nullptr) {
                return TestResult::Error;
            }
            buffers.push_back(buffer);
        }

        for (auto i = 0u; i < buffers.size(); i++) {
            if (this->verifyEnabled && this->arguments.type == StreamMemoryType::Triad && i == 1 && this->arguments.contents == BufferContents::Random) {
                const auto y = StreamMemoryHelper::getInput(this->arguments.contents, 1);
                BufferContentsHelperSycl::fillBufferWithData(sycl, buffers[i], bufferSize, [&](uint8_t *data) { y.copy(data, 0, bufferSize); });
            } else {
                BufferContentsHelperSycl::fillBuffer(sycl, buffers[i], bufferSize, arguments.contents);
            }
        }

        if (this->verifyEnabled) {
            this->outputChunk.resize(std::min(this->bufferSize, VerificationHelper::chunkSize));
            this->verifier.emplace(this->arguments.type, this->arguments.contents, this->bufferSize, sizeof(StreamT), this->arguments.partialMultiplier, streamMemoryScalarValue);
        }
        return TestResult::Success;
    }

    ~StreamMemoryBenchmark() {
        for (void *buffer : buffers) {
            UsmHelper::deallocate(arguments.memoryPlacement, sycl, buffer);
        }
    }

    sycl::event run() {
        switch (arguments.type) {
        case StreamMemoryType::Read:
            return this->verifyEnabled ? runReadStore() : runRead();
        case StreamMemoryType::Write:
            return arguments.contents == BufferContents::Random ? runWriteRandom() : runWrite();
        case StreamMemoryType::Scale:
            return runScale();
        case StreamMemoryType::Triad:
            return runTriad();
        case StreamMemoryType::Unknown:
            break;
        }
        FATAL_ERROR("Unknown StreamMemoryType");
    }

    void poisonOutput() {
        BufferContentsHelperSycl::fillBufferWithPattern(this->sycl, this->buffers.back(), this->bufferSize, VerificationHelper::poisonByte);
    }

    TestResult verify() {
        const auto readChunk = [this](size_t offset, size_t size) -> const uint8_t * {
            BufferContentsHelperSycl::copyToHost(this->sycl, this->outputChunk.data(), static_cast<const uint8_t *>(this->buffers.back()) + offset, size);
            return this->outputChunk.data();
        };
        return this->verifier->verify(readChunk);
    }

  private:
    size_t bufferCount() const {
        switch (arguments.type) {
        case StreamMemoryType::Read:
            return this->verifyEnabled ? 2 : 1;
        case StreamMemoryType::Write:
            return 1;
        case StreamMemoryType::Scale:
            return 2;
        case StreamMemoryType::Triad:
            return 3;
        case StreamMemoryType::Unknown:
            break;
        }
        FATAL_ERROR("Unknown StreamMemoryType");
    }

    sycl::event runRead() {
        const volatile StreamT *x = static_cast<const StreamT *>(buffers[0]);
        const uint32_t multiplier = arguments.partialMultiplier;

        return sycl.queue.submit([&](sycl::handler &cgh) {
            cgh.parallel_for(ndRange, [=](sycl::nd_item<1> item) {
                uint32_t i = {};
                if (!multipliedIndex(item, multiplier, i)) {
                    return;
                }
                // Without volatile the load is dead code and the kernel measures nothing.
                [[maybe_unused]] const StreamT value = x[i];
            });
        });
    }

    sycl::event runReadStore() {
        const StreamT *x = static_cast<const StreamT *>(this->buffers[0]);
        StreamT *output = static_cast<StreamT *>(this->buffers[1]);
        const uint32_t multiplier = this->arguments.partialMultiplier;

        return this->sycl.queue.submit([&](sycl::handler &cgh) {
            cgh.parallel_for(this->ndRange, [=](sycl::nd_item<1> item) {
                uint32_t i = {};
                if (!multipliedIndex(item, multiplier, i)) {
                    return;
                }
                output[i] = x[i];
            });
        });
    }

    sycl::event runWrite() {
        StreamT *x = static_cast<StreamT *>(buffers[0]);
        const StreamT scalar = scalarValue;
        const uint32_t multiplier = arguments.partialMultiplier;

        return sycl.queue.submit([&](sycl::handler &cgh) {
            cgh.parallel_for(ndRange, [=](sycl::nd_item<1> item) {
                uint32_t i = {};
                if (!multipliedIndex(item, multiplier, i)) {
                    return;
                }
                x[i] = scalar;
            });
        });
    }

    // Incompressible data, so this measures raw write bandwidth rather than memory compression.
    // The pattern has to be read with one vector load; a per-lane copy costs 75% of the bandwidth.
    sycl::event runWriteRandom() {
        StreamT *x = static_cast<StreamT *>(buffers[0]);

        return sycl.queue.submit([&](sycl::handler &cgh) {
            cgh.parallel_for(ndRange, [=](sycl::nd_item<1> item) {
                constexpr uint32_t patternSize = sizeof(StreamMemoryHelper::writeRandomPattern);
                const uint32_t i = static_cast<uint32_t>(item.get_global_id(0));
                const uint32_t dataIndex = i % (patternSize / sizeof(StreamT));
                StreamT value;
                __builtin_memcpy(&value, StreamMemoryHelper::writeRandomPattern + dataIndex * sizeof(StreamT), sizeof(StreamT));
                x[i] = value;
            });
        });
    }

    sycl::event runScale() {
        const StreamT *x = static_cast<const StreamT *>(buffers[0]);
        StreamT *y = static_cast<StreamT *>(buffers[1]);
        const StreamT scalar = scalarValue;

        return sycl.queue.submit([&](sycl::handler &cgh) {
            cgh.parallel_for(ndRange, [=](sycl::nd_item<1> item) {
                const uint32_t i = static_cast<uint32_t>(item.get_global_id(0));
                y[i] = x[i] * scalar;
            });
        });
    }

    sycl::event runTriad() {
        const StreamT *x = static_cast<const StreamT *>(buffers[0]);
        const StreamT *y = static_cast<const StreamT *>(buffers[1]);
        StreamT *z = static_cast<StreamT *>(buffers[2]);
        const StreamT scalar = scalarValue;

        return sycl.queue.submit([&](sycl::handler &cgh) {
            cgh.parallel_for(ndRange, [=](sycl::nd_item<1> item) {
                const uint32_t i = static_cast<uint32_t>(item.get_global_id(0));
                z[i] = x[i] + y[i] * scalar;
            });
        });
    }

    Sycl &sycl;
    const StreamMemoryArguments &arguments;
    const bool verifyEnabled;
    size_t bufferSize;
    sycl::nd_range<1> ndRange;
    StreamT scalarValue = {};
    std::vector<void *> buffers;
    std::vector<uint8_t> outputChunk;
    std::optional<StreamMemoryHelper::Verifier> verifier;
};

} // namespace

template <int vectorSize>
static TestResult runImpl(const StreamMemoryArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::GigabytesPerSecond, arguments.useEvents ? MeasurementType::Gpu : MeasurementType::Cpu);

    if (isStreamMemoryUnsupported(arguments)) {
        return TestResult::NoImplementation;
    }

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    StreamMemoryDispatch dispatch{};
    if (const TestResult result = getStreamMemoryDispatch(arguments, dispatch); result != TestResult::Success) {
        return result;
    }

    Sycl sycl{sycl::property::queue::enable_profiling{}};

    if (arguments.lws > sycl.device.get_info<sycl::info::device::max_work_group_size>()) {
        return TestResult::DeviceNotCapable;
    }

    StreamMemoryBenchmark<vectorSize> benchmark{sycl, arguments, dispatch};
    if (const TestResult result = benchmark.initBuffers(); result != TestResult::Success) {
        return result;
    }

    Timer timer;
    const bool verify = Configuration::get().verify;
    const size_t transferSize = getStreamMemoryTransferSize(arguments, dispatch.bufferSize);

    for (auto i = 0u; i < arguments.iterations; i++) {
        if (verify) {
            benchmark.poisonOutput();
        }
        timer.measureStart();
        sycl::event event = benchmark.run();
        event.wait();
        timer.measureEnd();

        if (arguments.useEvents) {
            auto startTime = event.get_profiling_info<sycl::info::event_profiling::command_start>();
            auto endTime = event.get_profiling_info<sycl::info::event_profiling::command_end>();
            auto timeNs = endTime - startTime;
            statistics.pushValue(std::chrono::nanoseconds(timeNs), transferSize, typeSelector.getUnit(), typeSelector.getType());
        } else {
            statistics.pushValue(timer.get(), transferSize, typeSelector.getUnit(), typeSelector.getType());
        }

        if (verify) {
            if (const TestResult result = benchmark.verify(); result != TestResult::Success) {
                return result;
            }
        }
    }
    return TestResult::Success;
}

static TestResult run(const StreamMemoryArguments &arguments, Statistics &statistics) {
    switch (arguments.vectorSize) {
    case 1:
        return runImpl<1>(arguments, statistics);
    case 2:
        return runImpl<2>(arguments, statistics);
    case 4:
        return runImpl<4>(arguments, statistics);
    case 8:
        return runImpl<8>(arguments, statistics);
    case 16:
        return runImpl<16>(arguments, statistics);
    default:
        return TestResult::InvalidArgs;
    }
}

[[maybe_unused]] static RegisterTestCaseImplementation<StreamMemory> registerTestCase(run, Api::SYCL);
