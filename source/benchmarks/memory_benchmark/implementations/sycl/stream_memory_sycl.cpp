/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/sycl/sycl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/memory_constants.h"
#include "framework/utility/timer.h"

#include "definitions/stream_memory.h"
#include "stream_memory_verification.h"

#include <algorithm>
#include <gtest/gtest.h>
#include <vector>

using namespace MemoryConstants;

template <typename T>
TestResult verifyBuffer(sycl::queue &queue, sycl::buffer<T, 1> &buffer, T expected) {
    std::vector<T> output(std::min(buffer.size(), VerificationHelper::chunkSize / sizeof(T)));
    const auto readChunk = [&](size_t offset, size_t size) -> const void * {
        queue.submit([&](sycl::handler &cgh) {
                 auto source = buffer.template get_access<sycl::access_mode::read>(cgh, sycl::range<1>{size / sizeof(T)}, sycl::id<1>{offset / sizeof(T)});
                 cgh.copy(source, output.data());
             })
            .wait();
        return output.data();
    };
    return VerificationHelper::verifyUniformOutput(buffer.size() * sizeof(T), expected, readChunk);
}

class StreamMemoryBenchmark {
  public:
    virtual ~StreamMemoryBenchmark() = default;
    virtual sycl::event run(sycl::queue &) = 0;
    virtual size_t transferSize() const = 0;
    virtual TestResult verify(sycl::queue &) = 0;
};

// Read benchmark

template <typename FloatingPointType>
class ReadKernel;

template <typename FloatingPointType>
class ReadBenchmark : public StreamMemoryBenchmark {
  public:
    ReadBenchmark(size_t bufferSize, FloatingPointType fillValue)
        : buffer(bufferSize, fillValue), fillValue(fillValue), deviceBuffer(this->buffer.data(), sycl::range<1>{bufferSize}), dummyOutputBuf(&dummyOutputVar, sycl::range<1>{1u}) {}

    sycl::event run(sycl::queue &queue) override {
        auto commandList = [&](sycl::handler &cgh) {
            const size_t bufferSize = this->buffer.size();
            auto x = this->deviceBuffer.template get_access<sycl::access_mode::read>(cgh);
            auto dummyOutput = this->dummyOutputBuf.template get_access<sycl::access_mode::discard_write>(cgh);
            FloatingPointType dummyValue = 0.37221;

            cgh.parallel_for<ReadKernel<FloatingPointType>>(
                sycl::range<1>{bufferSize},
                [=](sycl::item<1> item) {
                    FloatingPointType value = x[item];

                    // A trick to ensure compiler won't optimize away the read
                    if (value == dummyValue) {
                        dummyOutput[0] = value;
                    }
                });
        };
        return queue.submit(commandList);
    }

    size_t transferSize() const override {
        return this->buffer.size() * sizeof(FloatingPointType);
    }

    TestResult verify(sycl::queue &queue) override {
        return verifyBuffer(queue, this->deviceBuffer, this->fillValue);
    }

  private:
    FloatingPointType dummyOutputVar = {};
    std::vector<FloatingPointType> buffer;
    const FloatingPointType fillValue;
    sycl::buffer<FloatingPointType, 1> deviceBuffer;
    sycl::buffer<FloatingPointType, 1> dummyOutputBuf;
};

// Write benchmark

template <typename FloatingPointType>
class WriteKernel;

template <typename FloatingPointType>
class WriteBenchmark : public StreamMemoryBenchmark {
  public:
    WriteBenchmark(size_t bufferSize, FloatingPointType fillValue, FloatingPointType scalarValue)
        : buffer(bufferSize, fillValue), scalarValue(scalarValue), deviceBuffer(this->buffer.data(), sycl::range<1>{bufferSize}) {}

    sycl::event run(sycl::queue &queue) override {
        auto commandList = [&](sycl::handler &cgh) {
            const size_t bufferSize = this->buffer.size();
            const FloatingPointType scalarValue = this->scalarValue;
            auto x = this->deviceBuffer.template get_access<sycl::access_mode::discard_write>(cgh);

            cgh.parallel_for<WriteKernel<FloatingPointType>>(
                sycl::range<1>{bufferSize},
                [=](sycl::item<1> item) {
                    x[item] = scalarValue;
                });
        };
        return queue.submit(commandList);
    }

    size_t transferSize() const override {
        return this->buffer.size() * sizeof(FloatingPointType);
    }

    TestResult verify(sycl::queue &queue) override {
        return verifyBuffer(queue, this->deviceBuffer, this->scalarValue);
    }

  private:
    std::vector<FloatingPointType> buffer;
    const FloatingPointType scalarValue;
    sycl::buffer<FloatingPointType, 1> deviceBuffer;
};

// Scale benchmark

template <typename FloatingPointType>
class ScaleKernel;

template <typename FloatingPointType>
class ScaleBenchmark : public StreamMemoryBenchmark {
  public:
    ScaleBenchmark(size_t bufferSize, FloatingPointType fillValue, FloatingPointType scalarValue)
        : bufferX(bufferSize, fillValue), bufferY(bufferSize, fillValue), fillValue(fillValue), scalarValue(scalarValue), deviceBufferX(this->bufferX.data(), sycl::range<1>{bufferSize}), deviceBufferY(this->bufferY.data(), sycl::range<1>{bufferSize}) {}

    sycl::event run(sycl::queue &queue) override {
        auto commandList = [&](sycl::handler &cgh) {
            const size_t bufferSize = this->bufferX.size();
            const FloatingPointType scalarValue = this->scalarValue;
            auto x = this->deviceBufferX.template get_access<sycl::access_mode::discard_write>(cgh);
            auto y = this->deviceBufferY.template get_access<sycl::access_mode::read>(cgh);

            cgh.parallel_for<ScaleKernel<FloatingPointType>>(
                sycl::range<1>{bufferSize},
                [=](sycl::item<1> item) {
                    x[item] = y[item] * scalarValue;
                });
        };
        return queue.submit(commandList);
    }

    size_t transferSize() const override {
        return this->bufferX.size() * sizeof(FloatingPointType) * 2;
    }

    TestResult verify(sycl::queue &queue) override {
        return verifyBuffer(queue, this->deviceBufferX, this->fillValue * this->scalarValue);
    }

  private:
    std::vector<FloatingPointType> bufferX;
    std::vector<FloatingPointType> bufferY;
    const FloatingPointType fillValue;
    const FloatingPointType scalarValue;
    sycl::buffer<FloatingPointType, 1> deviceBufferX;
    sycl::buffer<FloatingPointType, 1> deviceBufferY;
};

// Triad benchmark

template <typename FloatingPointType>
class TriadKernel;

template <typename FloatingPointType>
class TriadBenchmark : public StreamMemoryBenchmark {
  public:
    TriadBenchmark(size_t bufferSize, FloatingPointType fillValue, FloatingPointType secondFillValue, FloatingPointType scalarValue)
        : bufferX(bufferSize, fillValue), bufferY(bufferSize, secondFillValue), bufferZ(bufferSize, fillValue), fillValue(fillValue), secondFillValue(secondFillValue), scalarValue(scalarValue), deviceBufferX(this->bufferX.data(), sycl::range<1>{bufferSize}), deviceBufferY(this->bufferY.data(), sycl::range<1>{bufferSize}), deviceBufferZ(this->bufferZ.data(), sycl::range<1>{bufferSize}) {}

    sycl::event run(sycl::queue &queue) override {
        auto commandList = [&](sycl::handler &cgh) {
            const size_t bufferSize = this->bufferX.size();
            const FloatingPointType scalarValue = this->scalarValue;
            auto x = this->deviceBufferX.template get_access<sycl::access_mode::read>(cgh);
            auto y = this->deviceBufferY.template get_access<sycl::access_mode::read>(cgh);
            auto z = this->deviceBufferZ.template get_access<sycl::access_mode::discard_write>(cgh);

            cgh.parallel_for<TriadKernel<FloatingPointType>>(
                sycl::range<1>{bufferSize},
                [=](sycl::item<1> item) {
                    z[item] = x[item] + y[item] * scalarValue;
                });
        };
        return queue.submit(commandList);
    }

    size_t transferSize() const override {
        return this->bufferX.size() * sizeof(FloatingPointType) * 3;
    }

    TestResult verify(sycl::queue &queue) override {
        return verifyBuffer(queue, this->deviceBufferZ, this->fillValue + this->secondFillValue * this->scalarValue);
    }

  private:
    std::vector<FloatingPointType> bufferX;
    std::vector<FloatingPointType> bufferY;
    std::vector<FloatingPointType> bufferZ;
    const FloatingPointType fillValue;
    const FloatingPointType secondFillValue;
    const FloatingPointType scalarValue;
    sycl::buffer<FloatingPointType, 1> deviceBufferX;
    sycl::buffer<FloatingPointType, 1> deviceBufferY;
    sycl::buffer<FloatingPointType, 1> deviceBufferZ;
};

static TestResult run(const StreamMemoryArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::GigabytesPerSecond, arguments.useEvents ? MeasurementType::Gpu : MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    Sycl sycl{sycl::property::queue::enable_profiling{}};

    Timer timer;
    bool useDoubles = sycl.device.has(sycl::aspect::fp64);

    const size_t elementSize = useDoubles ? sizeof(double) : sizeof(float);
    const size_t fillValue = 313u;
    const bool verify = Configuration::get().verify;
    const size_t secondFillValue = verify ? 17u : fillValue;
    const int32_t scalarValue = -999;

    std::unique_ptr<StreamMemoryBenchmark> benchmark = {};
    switch (arguments.type) {
    case StreamMemoryType::Read:
        if (useDoubles) {
            benchmark = std::make_unique<ReadBenchmark<double>>(arguments.size / elementSize, static_cast<double>(fillValue));
        } else {
            benchmark = std::make_unique<ReadBenchmark<float>>(arguments.size / elementSize, static_cast<float>(fillValue));
        }
        break;
    case StreamMemoryType::Write:
        if (useDoubles) {
            benchmark = std::make_unique<WriteBenchmark<double>>(arguments.size / elementSize, static_cast<double>(fillValue), static_cast<double>(scalarValue));
        } else {
            benchmark = std::make_unique<WriteBenchmark<float>>(arguments.size / elementSize, static_cast<float>(fillValue), static_cast<float>(scalarValue));
        }
        break;
    case StreamMemoryType::Scale:
        if (useDoubles) {
            benchmark = std::make_unique<ScaleBenchmark<double>>(arguments.size / elementSize, static_cast<double>(fillValue), static_cast<double>(scalarValue));
        } else {
            benchmark = std::make_unique<ScaleBenchmark<float>>(arguments.size / elementSize, static_cast<float>(fillValue), static_cast<float>(scalarValue));
        }
        break;
    case StreamMemoryType::Triad:
        if (useDoubles) {
            benchmark = std::make_unique<TriadBenchmark<double>>(arguments.size / elementSize, static_cast<double>(fillValue), static_cast<double>(secondFillValue), static_cast<double>(scalarValue));
        } else {
            benchmark = std::make_unique<TriadBenchmark<float>>(arguments.size / elementSize, static_cast<float>(fillValue), static_cast<float>(secondFillValue), static_cast<float>(scalarValue));
        }
        break;
    default:
        FATAL_ERROR("Unknown StreamMemoryType");
    }

    // Warm-up
    auto event = benchmark->run(sycl.queue);
    event.wait();

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        auto event = benchmark->run(sycl.queue);
        event.wait();
        timer.measureEnd();
        if (arguments.useEvents) {
            auto startTime = event.get_profiling_info<sycl::info::event_profiling::command_start>();
            auto endTime = event.get_profiling_info<sycl::info::event_profiling::command_end>();
            auto timeNs = endTime - startTime;
            statistics.pushValue(std::chrono::nanoseconds(timeNs), benchmark->transferSize(), typeSelector.getUnit(), typeSelector.getType());
        } else {
            statistics.pushValue(timer.get(), benchmark->transferSize(), typeSelector.getUnit(), typeSelector.getType());
        }

        if (verify) {
            if (const TestResult result = benchmark->verify(sycl.queue); result != TestResult::Success) {
                return result;
            }
        }
    }
    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<StreamMemory> registerTestCase(run, Api::SYCL);
