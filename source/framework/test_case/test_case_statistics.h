/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "framework/configuration.h"
#include "framework/utility/statistics.h"

#include <map>
#include <memory>
#include <string>

class TestCaseStatistics : public Statistics {
  public:
    using Clock = std::chrono::high_resolution_clock;
    using Value = double;
    using SamplesVector = std::vector<Value>;
    struct Samples {
        MeasurementUnit unit = MeasurementUnit::Unknown;
        MeasurementType type = MeasurementType::Unknown;
        SamplesVector vector = {};
    };
    using SamplesMap = std::map<std::string, Samples>;

    explicit TestCaseStatistics(size_t maxSamplesCount, Configuration::PrintType printType);

    void pushValue(Clock::duration time, MeasurementUnit unit, MeasurementType type, const std::string &description = "") override;
    void pushValue(Clock::duration time, uint64_t size, MeasurementUnit unit, MeasurementType type, const std::string &description = "") override;

    bool isEmpty() const override;
    bool isFull() const override;

    static void printStatisticsHeader(Configuration::PrintType printType);
    void printStatisticsBeforeTest(const std::string &testCaseName) const;
    void printClearLineAfterTest() const;
    void printStatistics(const std::string &testCaseName) const;
    void printStatisticsString(const std::string &testCaseName, const std::string &message, char lineEnding = '\n') const;

  private:
    static void overrideMeasurementUnit(MeasurementUnit &unit);
    void pushValue(Value value, const std::string &description, MeasurementUnit unit, MeasurementType type);
    void printStatisticsDefault(const std::string &testCaseName) const;
    void printStatisticsCsv(const std::string &testCaseName) const;
    void printStatisticsVerbose() const;

    const Configuration::PrintType printType;
    SamplesMap samplesMap = {};
    bool reachedInfinity = false;

    struct Metrics;
    struct MetricsStrings;
};

struct TestCaseStatistics::Metrics {
    explicit Metrics(const SamplesVector &samples);
    Value min;
    Value max;
    Value mean;
    Value median;
    Value standardDeviation;

  private:
    static Value calculateMin(const SamplesVector &samples);
    static Value calculateMax(const SamplesVector &samples);
    static Value calculateMean(const SamplesVector &samples);
    static Value calculateMedian(const SamplesVector &samples);
    static Value calculateStandardDeviation(const SamplesVector &samples, Value mean);
};

struct TestCaseStatistics::MetricsStrings {
    MetricsStrings(const std::string &name, const Samples &samples, bool reachedInfinity);
    Metrics metrics;
    std::string min;
    std::string max;
    std::string mean;
    std::string median;
    std::string standardDeviation;
    std::string type;
    std::string label;

  private:
    static std::string generateMin(Value min);
    static std::string generateMax(Value max);
    static std::string generateMean(Value mean, bool reachedInfinity);
    static std::string generateMedian(Value median);
    static std::string generateStandardDeviation(Value standardDeviation, bool reachedInfinity);
    static std::string generate(Value value);
    static std::string generateLabel(const std::string &name, MeasurementUnit unit);
};
