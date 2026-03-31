/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_case_statistics.h"

#include "framework/argument/abstract/argument.h"
#include "framework/benchmark_info.h"
#include "framework/test_map.h"
#include "framework/utility/error.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <type_traits>

TestCaseStatistics::TestCaseStatistics(size_t maxSamplesCount, Configuration::PrintType printType)
    : Statistics(maxSamplesCount),
      printType(printType) {
}

void TestCaseStatistics::pushPercentage(double value, MeasurementUnit unit, MeasurementType type, const std::string &description) {
    if (unit != MeasurementUnit::Percentage) {
        FATAL_ERROR("Incorrect measurement unit");
    }

    overrideMeasurementUnit(unit);

    const Value percentage = value;
    this->pushValue(percentage, description, unit, type);
}

void TestCaseStatistics::pushValue(Clock::duration time, MeasurementUnit unit, MeasurementType type, const std::string &description) {
    static_assert(std::is_floating_point_v<Value>, "Need floating point type for the below cast to work properly");
    const Value timeSeconds = std::chrono::duration_cast<std::chrono::duration<Value>>(time).count();

    overrideMeasurementUnit(unit);

    switch (unit) {
    case MeasurementUnit::Nanoseconds: {
        const Value timeNanoseconds = timeSeconds * 1e9;
        this->pushValue(timeNanoseconds, description, unit, type);
        break;
    }
    case MeasurementUnit::Microseconds: {
        const Value timeMicroseconds = timeSeconds * 1e6;
        this->pushValue(timeMicroseconds, description, unit, type);
        break;
    }
    case MeasurementUnit::Latency: {
        const Value latencyVal = timeSeconds * 1e9;
        this->pushValue(latencyVal, description, unit, type);
        break;
    }
    case MeasurementUnit::GigabytesPerSecond:
        FATAL_ERROR("Buffer size needs to be passed when unit is ", std::to_string(unit));
    default:
        FATAL_ERROR("Unknown measurement unit");
    }
}

void TestCaseStatistics::pushValue(Clock::duration time, uint64_t size, MeasurementUnit unit, MeasurementType type, const std::string &description) {
    static_assert(std::is_floating_point_v<Value>, "Need floating point type for the below cast to work properly");
    if (unit != MeasurementUnit::GigabytesPerSecond) {
        FATAL_ERROR("Test is passing size which requires Bandwidth calculcation, please fix benchmark");
    }

    const Value timeSeconds = std::chrono::duration_cast<std::chrono::duration<Value>>(time).count();

    overrideMeasurementUnit(unit);

    switch (unit) {
    case MeasurementUnit::Microseconds: {
        const Value timeMicroseconds = timeSeconds * 1e6;
        this->pushValue(timeMicroseconds, description, unit, type);
        break;
    }
    case MeasurementUnit::GigabytesPerSecond: {
        const Value timeNanoseconds = timeSeconds * 1e9;
        const Value bandwidth = size / timeNanoseconds; // Bytes/Nanoseconds = Gigabytes/Seconds
        this->pushValue(bandwidth, description, unit, type);
        break;
    }
    default:
        FATAL_ERROR("Unknown measurement unit");
    }
}

void TestCaseStatistics::pushCpuCounter(uint64_t count, MeasurementUnit unit, MeasurementType type, const std::string &description) {
    switch (unit) {
    case MeasurementUnit::CpuHardwareCounter: {
        const Value cpuCounter = static_cast<Value>(count);
        this->pushValue(cpuCounter, description, unit, type);
        break;
    }
    default:
        FATAL_ERROR("Unknown measurement unit");
    }
}

void TestCaseStatistics::pushEnergy(size_t microJoules, MeasurementUnit unit, MeasurementType type, const std::string &description) {
    switch (unit) {
    case MeasurementUnit::MicroJoules: {
        const Value energyMicroJoules = static_cast<Value>(microJoules);
        this->pushValue(energyMicroJoules, description, unit, type);
        break;
    }
    default:
        FATAL_ERROR("Unknown measurement unit");
    }
}

void TestCaseStatistics::pushEnergy(double watts, MeasurementUnit unit, MeasurementType type, const std::string &description) {
    switch (unit) {
    case MeasurementUnit::Watts: {
        const Value powerWatts = static_cast<Value>(watts);
        this->pushValue(powerWatts, description, unit, type);
        break;
    }
    default:
        FATAL_ERROR("Unknown measurement unit");
    }
}

void TestCaseStatistics::pushUnitAndType(MeasurementUnit unit, MeasurementType type) {
    overrideMeasurementUnit(unit);

    this->noopSample.unit = unit;
    this->noopSample.type = type;
}

bool TestCaseStatistics::isEmpty() const {
    for (auto &samplesEntry : samplesMap) {
        if (samplesEntry.second.vector.size() != 0) {
            return false;
        }
    }
    return true;
}

bool TestCaseStatistics::isFull() const {
    DEVELOPER_WARNING_IF(samplesMap.size() == 0, "Test did not generate any values");
    for (auto &samplesEntry : samplesMap) {
        if (samplesEntry.second.vector.size() != maxSamplesCount) {
            return false;
        }
    }
    return true;
}

void TestCaseStatistics::overrideMeasurementUnit(MeasurementUnit &unit) {
    if (unit == MeasurementUnit::GigabytesPerSecond && Configuration::get().doNotPrintBandwidth) {
        unit = MeasurementUnit::Microseconds;
    }
}

void TestCaseStatistics::pushValue(Value value, const std::string &description, MeasurementUnit unit, MeasurementType type) {
    FATAL_ERROR_IF(unit == MeasurementUnit::Unknown, "Concrete MeasurementUnit has to be specified");
    FATAL_ERROR_IF(type == MeasurementType::Unknown, "Concrete MeasurementType has to be specified");

    auto &samples = this->samplesMap[description];

    // We expect a precise amount of measurements requested by the user.
    FATAL_ERROR_IF(samples.vector.size() == maxSamplesCount, "Too many values pushed by the test");
    samples.vector.reserve(maxSamplesCount);

    // Set unit and type for the samples
    if (samples.unit != unit) {
        FATAL_ERROR_IF(samples.unit != MeasurementUnit::Unknown, "Different units used for the same measurement");
        samples.unit = unit;
    }
    if (samples.type != type) {
        FATAL_ERROR_IF(samples.type != MeasurementType::Unknown, "Different types used for the same measurement");
        samples.type = type;
    }

    samples.vector.push_back(value);
    if (value >= std::numeric_limits<double>::max()) {
        this->reachedInfinity = true;
    }
}

struct ColumnInfo {
    int width;
    const char *label;

    using Columns = std::array<ColumnInfo, 8>;

    static Columns getColumns() {
        return {{
            {100, "TestCase"},
            {15, "Mean"},
            {15, "Median"},
            {15, "StdDev"},
            {15, "Min"},
            {15, "Max"},
            {7, "Type"},
            {15, "Label [unit]"},
        }};
    }
};

std::vector<TestCaseStatistics::BufferedLine> TestCaseStatistics::testResults;
int TestCaseStatistics::lastTransientLineWidth = 0;
std::string TestCaseStatistics::deviceInfo;

void TestCaseStatistics::setDeviceInfo(const std::string &info) {
    deviceInfo = info;
}

static std::string htmlEscape(const std::string &text) {
    std::string escaped;
    escaped.reserve(text.size());
    for (const char c : text) {
        switch (c) {
        case '&':
            escaped += "&amp;";
            break;
        case '<':
            escaped += "&lt;";
            break;
        case '>':
            escaped += "&gt;";
            break;
        case '"':
            escaped += "&quot;";
            break;
        default:
            escaped += c;
        }
    }
    return escaped;
}

// Extract and trim the column at colIndex from a BufferedLine::results string
// built with fixed-width right-aligned columns. colIndex matches the ColumnInfo
// index (columns[0] is the name column, columns[1..7] are metric columns).
static std::string extractColumn(const std::string &results, const ColumnInfo::Columns &columns, size_t colIndex) {
    int offset = 0;
    for (size_t i = 1; i < colIndex; i++) {
        offset += columns[i].width;
    }
    if (static_cast<size_t>(offset) >= results.size()) {
        return {};
    }
    std::string val = results.substr(offset, columns[colIndex].width);
    auto start = val.find_first_not_of(' ');
    if (start == std::string::npos) {
        return {};
    }
    auto end = val.find_last_not_of(' ');
    return val.substr(start, end - start + 1);
}

void TestCaseStatistics::printStatisticsHeader(Configuration::PrintType printType, int nameColumnWidth) {
    const auto columns = ColumnInfo::getColumns();
    const auto columnCount = columns.size();
    const int nameWidth = nameColumnWidth > 0 ? nameColumnWidth : columns[0].width;
    switch (printType) {
    case Configuration::PrintType::DefaultWithVerbose:
    case Configuration::PrintType::Default:
    case Configuration::PrintType::Noop: {
        std::cout << std::left << std::setw(nameWidth) << columns[0].label << std::right;
        for (size_t i = 1; i < columnCount; i++) {
            std::cout << std::setw(columns[i].width) << columns[i].label;
        }
        std::cout << std::endl;
        break;
    }
    case Configuration::PrintType::Csv:
        for (auto columnIndex = 0u; columnIndex < columnCount; columnIndex++) {
            const ColumnInfo &column = columns[columnIndex];
            std::cout << column.label;
            if (columnIndex != columnCount - 1) {
                std::cout << ",";
            }
        }
        std::cout << std::endl;
        break;
    default:
        FATAL_ERROR("unknown print type selected");
    }
}

void TestCaseStatistics::printStatisticsBeforeTest(const std::string &testCaseName) const {
    if (Configuration::get().interactivePrints) {
        // With buffered output, just print the name on its own line so the user can see progress
        std::cout << testCaseName << '\n'
                  << std::flush;
    } else {
        // Ending line with carriage return instead of newline will cause the next print to overwrite this line
        printStatisticsString(testCaseName, "", '\r');
    }
}

void TestCaseStatistics::printClearLineAfterTest() const {
    if (Configuration::get().interactivePrints) {
        return;
    }
    std::cout << std::string(lastTransientLineWidth, ' ') << '\r';
    lastTransientLineWidth = 0;
}

void TestCaseStatistics::printStatistics(const std::string &testCaseName) const {
    switch (printType) {
    case Configuration::PrintType::Default:
        printStatisticsDefault(testCaseName);
        break;
    case Configuration::PrintType::DefaultWithVerbose:
        printStatisticsDefault(testCaseName);
        printStatisticsVerbose();
        break;
    case Configuration::PrintType::Noop:
        printStatisticsNoop(testCaseName);
        break;
    case Configuration::PrintType::Csv:
        printStatisticsCsv(testCaseName);
        break;
    default:
        FATAL_ERROR("unknown print type selected");
    }
}

void TestCaseStatistics::printStatisticsDefault(const std::string &testCaseName) const {
    const auto columns = ColumnInfo::getColumns();

    bool isFirst = true;
    for (const auto &samplesEntry : this->samplesMap) {
        const std::string &samplesName = samplesEntry.first;
        const Samples &samples = samplesEntry.second;
        const MetricsStrings metricsStrings{samplesName, samples, this->reachedInfinity, Configuration::get().warmupIterations, Configuration::get().trimOutliers};

        int column = 1; // skip name column -- handled separately
        std::ostringstream results;
        results << std::setw(columns[column++].width) << metricsStrings.mean;
        results << std::setw(columns[column++].width) << metricsStrings.median;
        results << std::setw(columns[column++].width) << metricsStrings.standardDeviation;
        results << std::setw(columns[column++].width) << metricsStrings.min;
        results << std::setw(columns[column++].width) << metricsStrings.max;
        results << std::setw(columns[column++].width) << metricsStrings.type;
        results << ' ' << std::setw(columns[column++].width - 1) << metricsStrings.label;

        testResults.push_back({isFirst ? testCaseName : "", results.str()});
        isFirst = false;
    }
}

void TestCaseStatistics::printStatisticsNoop(const std::string &testCaseName) const {
    const auto columns = ColumnInfo::getColumns();

    // Calculate left padding assuming Type and Label are the last two columns
    size_t paddingLeft = 0;
    for (size_t i = 1; i < columns.size() - 1; i++) {
        paddingLeft += columns[i].width;
    }

    std::ostringstream results;
    results << std::setw(paddingLeft) << std::to_string(this->noopSample.type);
    results << std::setw(columns.back().width) << std::to_string(this->noopSample.unit);

    testResults.push_back({testCaseName, results.str()});
}

void TestCaseStatistics::printStatisticsCsv(const std::string &testCaseName) const {
    FATAL_ERROR_IF(this->samplesMap.begin() == this->samplesMap.end(), "Test did not generate any values");

    for (const auto &samplesEntry : this->samplesMap) {
        const std::string &samplesName = samplesEntry.first;
        const Samples &samples = samplesEntry.second;
        const MetricsStrings metricsStrings{samplesName, samples, this->reachedInfinity, Configuration::get().warmupIterations, Configuration::get().trimOutliers};

        std::cout << testCaseName << ",";
        std::cout << metricsStrings.mean << ",";
        std::cout << metricsStrings.median << ",";
        std::cout << metricsStrings.standardDeviation << ",";
        std::cout << metricsStrings.min << ",";
        std::cout << metricsStrings.max << ",";
        std::cout << metricsStrings.type << ",";
        std::cout << metricsStrings.label;
        std::cout << std::endl;
    }
}

void TestCaseStatistics::printStatisticsVerbose() const {
    for (const auto &samplesEntry : this->samplesMap) {
        std::ostringstream line;
        line << "individual ";
        if (const auto &samplesName = samplesEntry.first; samplesName != "") {
            line << samplesName << ' ';
        }
        line << "results: [ ";
        for (const auto &sample : samplesEntry.second.vector) {
            line << sample << ' ';
        }
        line << "]";
        testResults.push_back({"", line.str(), true});
    }
    testResults.push_back({"", "", true});
}

void TestCaseStatistics::printStatisticsString(const std::string &testCaseName, const std::string &message, char lineEnding) const {
    const auto columns = ColumnInfo::getColumns();
    const auto columnCount = columns.size();
    switch (printType) {
    case Configuration::PrintType::DefaultWithVerbose:
    case Configuration::PrintType::Default:
    case Configuration::PrintType::Noop: {
        if (lineEnding == '\r') {
            // Transient before-test display -- print immediately
            const int nameWidth = std::max(columns[0].width, static_cast<int>(testCaseName.size()));
            std::cout << std::setw(nameWidth) << testCaseName;
            const size_t maxStringWidth = columns[1].width + columns[2].width + columns[3].width;
            std::cout << std::setw(maxStringWidth) << message;
            lastTransientLineWidth = nameWidth + static_cast<int>(maxStringWidth);
            std::cout << lineEnding;
        } else {
            // Final result line -- buffer for aligned flush
            const size_t maxStringWidth = columns[1].width + columns[2].width + columns[3].width;
            std::ostringstream results;
            results << std::setw(maxStringWidth) << message;
            testResults.push_back({testCaseName, results.str()});
        }
        break;
    }
    case Configuration::PrintType::Csv: {
        std::cout << testCaseName << ",";
        for (auto column = 1u; column < columnCount; column++) {
            std::cout << message;
            if (column != columnCount - 1) {
                std::cout << ",";
            }
        }
        std::cout << lineEnding;
        break;
    }
    default:
        FATAL_ERROR("unknown print type selected");
    }
}

void TestCaseStatistics::flushBufferedResults(Configuration::PrintType printType) {
    if (testResults.empty()) {
        return;
    }

    // Find the longest test case name across all buffered entries, but at least as wide as the header label
    const auto columns = ColumnInfo::getColumns();
    int maxNameWidth = static_cast<int>(std::strlen(columns[0].label));
    for (const auto &line : testResults) {
        if (!line.isFullLine) {
            maxNameWidth = std::max(maxNameWidth, static_cast<int>(line.name.size()));
        }
    }

    if (!Configuration::get().noColumnNames) {
        printStatisticsHeader(printType, maxNameWidth);
    }

    for (const auto &line : testResults) {
        if (line.isFullLine) {
            std::cout << line.results << '\n';
        } else {
            std::cout << std::setw(maxNameWidth) << std::left << line.name << std::right << line.results << '\n';
        }
    }
    std::cout.flush();

    const std::string &htmlPath = Configuration::get().htmlOutput;
    if (!htmlPath.empty()) {
        writeHtmlResults(htmlPath);
    }

    const std::string &mdPath = Configuration::get().mdOutput;
    if (!mdPath.empty()) {
        writeMdResults(mdPath);
    }

    testResults.clear();
}

static std::string currentTimestamp() {
    std::time_t t = std::time(nullptr);
    std::tm tm = {};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream timestamp;
    timestamp << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return timestamp.str();
}

void TestCaseStatistics::writeHtmlResults(const std::string &filePath) {
    std::ofstream htmlFile(filePath);
    if (!htmlFile) {
        std::cerr << "ERROR: cannot open HTML output file: " << filePath << '\n';
        return;
    }

    const std::string benchmarkName = BenchmarkInfo::get().getBenchmarkName();
    const auto &cfg = Configuration::get();

    htmlFile << "<!DOCTYPE html>\n<html>\n<head>\n"
             << "<meta charset=\"utf-8\">\n"
             << "<title>" << htmlEscape(benchmarkName) << " results</title>\n"
             << "<style>\n"
             << "body{font-family:sans-serif;margin:2em;}\n"
             << "h1{margin-bottom:0.2em;}\n"
             << "p.meta{color:#555;margin-top:0;}\n"
             << "table{border-collapse:collapse;width:100%;}\n"
             << "th{background:#333;color:#fff;padding:6px 12px;text-align:left;}\n"
             << "td{padding:5px 12px;border-bottom:1px solid #ddd;font-family:monospace;}\n"
             << "tr:nth-child(even){background:#f5f5f5;}\n"
             << "tr:hover{background:#fffbe6;}\n"
             << "</style>\n</head>\n<body>\n"
             << "<h1>" << htmlEscape(benchmarkName) << "</h1>\n"
             << "<p class=\"meta\">Iterations: " << cfg.iterations
             << " &nbsp;|&nbsp; Warmup: " << cfg.warmupIterations
             << " &nbsp;|&nbsp; " << currentTimestamp() << "</p>\n";

    if (!deviceInfo.empty()) {
        htmlFile << "<pre style=\"background:#f0f0f0;padding:0.8em;border-radius:4px;font-size:0.9em;white-space:pre-wrap;\">"
                 << htmlEscape(deviceInfo)
                 << "</pre>\n";
    }

    const auto columns = ColumnInfo::getColumns();

    htmlFile << "<table>\n<thead><tr>";
    for (const auto &col : columns) {
        htmlFile << "<th>" << col.label << "</th>";
    }
    htmlFile << "</tr></thead>\n<tbody>\n";

    for (const auto &line : testResults) {
        if (line.isFullLine) {
            continue;
        }
        htmlFile << "<tr><td>" << htmlEscape(line.name) << "</td>";
        for (size_t colIndex = 1; colIndex < columns.size(); colIndex++) {
            htmlFile << "<td>" << htmlEscape(extractColumn(line.results, columns, colIndex)) << "</td>";
        }
        htmlFile << "</tr>\n";
    }

    htmlFile << "</tbody>\n</table>\n";

    htmlFile << "<h2>Test Descriptions</h2>\n";
    for (const auto &entry : TestMap::get()) {
        const TestCaseInterface &testCase = *entry.second;
        if (testCase.getApisWithImplementation().empty()) {
            continue;
        }
        htmlFile << "<h3>" << htmlEscape(testCase.getTestCaseName()) << "</h3>\n"
                 << "<p>" << htmlEscape(testCase.getHelp()) << "</p>\n";
        const auto arguments = testCase.getArguments();
        const auto &args = arguments->getArguments();
        if (!args.empty()) {
            htmlFile << "<table>\n<thead><tr><th>Argument</th><th>Description</th></tr></thead>\n<tbody>\n";
            for (const Argument *arg : args) {
                htmlFile << "<tr><td>" << htmlEscape(arg->getKey()) << "</td>"
                         << "<td>" << htmlEscape(arg->getExtraHelp()) << "</td></tr>\n";
            }
            htmlFile << "</tbody>\n</table>\n";
        }
    }

    htmlFile << "</body>\n</html>\n";
}

void TestCaseStatistics::writeMdResults(const std::string &filePath) {
    std::ofstream mdFile(filePath);
    if (!mdFile) {
        std::cerr << "ERROR: cannot open Markdown output file: " << filePath << '\n';
        return;
    }

    const std::string benchmarkName = BenchmarkInfo::get().getBenchmarkName();
    const auto &cfg = Configuration::get();

    mdFile << "# " << benchmarkName << "\n\n"
           << "- **Iterations:** " << cfg.iterations << "\n"
           << "- **Warmup:** " << cfg.warmupIterations << "\n"
           << "- **Date:** " << currentTimestamp() << "\n\n";

    if (!deviceInfo.empty()) {
        mdFile << "```\n"
               << deviceInfo << "```\n\n";
    }

    const auto columns = ColumnInfo::getColumns();

    for (const auto &col : columns) {
        mdFile << "| " << col.label << " ";
    }
    mdFile << "|\n";
    for (size_t i = 0; i < columns.size(); i++) {
        mdFile << "|---";
    }
    mdFile << "|\n";

    for (const auto &line : testResults) {
        if (line.isFullLine) {
            continue;
        }
        mdFile << "| " << line.name;
        for (size_t colIndex = 1; colIndex < columns.size(); colIndex++) {
            mdFile << " | " << extractColumn(line.results, columns, colIndex);
        }
        mdFile << " |\n";
    }

    mdFile << "\n## Test Descriptions\n";
    for (const auto &entry : TestMap::get()) {
        const TestCaseInterface &testCase = *entry.second;
        if (testCase.getApisWithImplementation().empty()) {
            continue;
        }
        mdFile << "\n### " << testCase.getTestCaseName() << "\n\n"
               << testCase.getHelp() << "\n";
        const auto arguments = testCase.getArguments();
        const auto &args = arguments->getArguments();
        if (!args.empty()) {
            mdFile << "\n| Argument | Description |\n|---|---|\n";
            for (const Argument *arg : args) {
                mdFile << "| " << arg->getKey() << " | " << arg->getExtraHelp() << " |\n";
            }
        }
    }
}

TestCaseStatistics::Metrics::Metrics(const SamplesVector &samples, size_t iterationsToSkip, size_t trimPercentage) {
    ConstIter begin, end;
    SamplesVector trimmed;
    if (trimPercentage > 0) {
        trimmed = getTrimmedSamples(samples, iterationsToSkip, trimPercentage);
        begin = trimmed.begin();
        end = trimmed.end();
    } else {
        begin = samples.begin() + iterationsToSkip;
        end = samples.end();
    }
    mean = calculateMean(begin, end);
    min = calculateMin(begin, end);
    max = calculateMax(begin, end);
    median = calculateMedian(begin, end);
    standardDeviation = calculateStandardDeviation(begin, end, mean);
}

TestCaseStatistics::SamplesVector TestCaseStatistics::Metrics::getTrimmedSamples(const SamplesVector &samples, size_t iterationsToSkip, size_t trimPercentage) {
    SamplesVector result(samples.begin() + iterationsToSkip, samples.end());
    std::sort(result.begin(), result.end());
    const size_t trimCount = result.size() * trimPercentage / 100;
    result.erase(result.end() - trimCount, result.end());
    result.erase(result.begin(), result.begin() + trimCount);
    return result;
}

TestCaseStatistics::Value TestCaseStatistics::Metrics::calculateMin(ConstIter begin, ConstIter end) {
    return *std::min_element(begin, end);
}

TestCaseStatistics::Value TestCaseStatistics::Metrics::calculateMax(ConstIter begin, ConstIter end) {
    return *std::max_element(begin, end);
}

TestCaseStatistics::Value TestCaseStatistics::Metrics::calculateMean(ConstIter begin, ConstIter end) {
    return std::accumulate(begin, end, Value{0}) / std::distance(begin, end);
}

TestCaseStatistics::Value TestCaseStatistics::Metrics::calculateMedian(ConstIter begin, ConstIter end) {
    SamplesVector sorted(begin, end);
    std::sort(sorted.begin(), sorted.end());
    const auto samplesCount = sorted.size();
    if (samplesCount % 2 == 0) {
        return (sorted[samplesCount / 2 - 1] + sorted[samplesCount / 2]) / 2;
    } else {
        return sorted[samplesCount / 2];
    }
}

TestCaseStatistics::Value TestCaseStatistics::Metrics::calculateStandardDeviation(ConstIter begin, ConstIter end, Value mean) {
    Value diffSum = 0;
    for (auto it = begin; it != end; ++it) {
        const auto difference = *it - mean;
        diffSum += difference * difference;
    }
    const auto count = std::distance(begin, end);
    double stdDev = static_cast<double>(diffSum) / count;
    stdDev = std::sqrt(stdDev);
    stdDev /= mean;
    return stdDev;
}

TestCaseStatistics::MetricsStrings::MetricsStrings(const std::string &name, const Samples &samples, bool reachedInfinity, size_t iterationsToSkip, size_t trimPercentage)
    : metrics(samples.vector, iterationsToSkip, trimPercentage),
      min(generateMin(metrics.min)),
      max(generateMax(metrics.max)),
      mean(generateMean(metrics.mean, reachedInfinity)),
      median(generateMedian(metrics.median)),
      standardDeviation(generateStandardDeviation(metrics.standardDeviation, reachedInfinity)),
      type(std::to_string(samples.type)),
      label(generateLabel(name, samples.unit)) {
}

std::string TestCaseStatistics::MetricsStrings::generateMin(Value min) {
    return generate(min);
}

std::string TestCaseStatistics::MetricsStrings::generateMax(Value max) {
    return generate(max);
}

std::string TestCaseStatistics::MetricsStrings::generateMean(Value mean, bool reachedInfinity) {
    if (reachedInfinity) {
        return "inf";
    }
    return generate(mean);
}

std::string TestCaseStatistics::MetricsStrings::generateMedian(Value median) {
    return generate(median);
}

std::string TestCaseStatistics::MetricsStrings::generateStandardDeviation(Value standardDeviation, bool reachedInfinity) {
    if (reachedInfinity) {
        return "inf";
    }

    std::ostringstream result{};
    result << std::fixed << std::setprecision(2) << (100 * standardDeviation) << "%";
    return result.str();
}

std::string TestCaseStatistics::MetricsStrings::generate(Value value) {
    std::ostringstream result{};
    result << std::fixed << std::setprecision(3) << value;
    return result.str();
}

std::string TestCaseStatistics::MetricsStrings::generateLabel(const std::string &name, MeasurementUnit unit) {
    if (name.empty()) {
        return std::to_string(unit);
    }
    return name + " " + std::to_string(unit);
}
