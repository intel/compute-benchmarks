/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/argument/argument_container.h"
#include "framework/argument/string_argument.h"
#include "framework/utility/command_line_argument.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/process.h"

#include "data_types.h"
#include "helpers/helpers.h"
#include "helpers_additional.h"

#if __has_include(<filesystem>)
#include <filesystem>
namespace FileSystem = std::filesystem;
#else
#include <experimental/filesystem>
namespace FileSystem = std::experimental::filesystem;
#endif

int main(int argc, char **argv) {
    // Parse command-line arguments
    CommandLineArguments commandLineArguments = {};
    std::string commandLineArgumentsParsingErrors = {};
    if (!CommandLineArgument::parseArguments(argc, argv, commandLineArguments, commandLineArgumentsParsingErrors)) {
        std::cerr << commandLineArgumentsParsingErrors << std::endl;
        return 1;
    }
    ArgumentContainer arguments = {};
    StringArgument path{arguments, "path"};
    StringArgument fileName{arguments, "fileName"};
    arguments.parseArguments(commandLineArguments);

    // Full names and paths of benchmarks are known by CMake and passed via a preprocessor definition.
    const BenchmarkInstance benchmarkInstances[] = {TARGETS};

    // Group BenchmarkInstances by base name
    Benchmarks benchmarks = {};
    // And collect list of locations
    std::set<std::string> locations = {};
    for (const BenchmarkInstance &benchmarkInstance : benchmarkInstances) {
        Benchmark &benchmark = benchmarks[benchmarkInstance.baseName];
        locations.insert(benchmarkInstance.location);
        assignAndValidate(benchmark.baseName, benchmarkInstance.baseName, "setting base name from benchmark instance");
        if (std::find(benchmark.locations.begin(), benchmark.locations.end(), benchmarkInstance.location) == benchmark.locations.end()) {
            benchmark.locations.push_back(benchmarkInstance.location);
        }
        benchmark.instances.push_back(&benchmarkInstance);
    }

    // Fill all fields for each Benchmark
    std::cerr << "Gathering test information from binaries\n";
    for (auto &entry : benchmarks) {
        Benchmark &benchmark = entry.second;

        // Call every instance
        for (const BenchmarkInstance *benchmarkInstance : benchmark.instances) {
            std::cerr << "  " << benchmarkInstance->path << '\n';

            // Call the instance and get output
            Process process{benchmarkInstance->path};
            process.addArgument("generateDocs", "");
            std::cerr << "    Running\n";
            process.run();
            if (TestResult result = process.getResult(); result != TestResult::Success) {
                FATAL_ERROR("Generating docs from ", benchmarkInstance->path, " failed.\n", process.getStdout());
            }
            std::istringstream output{process.getStdout()};

            // Parse first line of output - benchmark name and description
            std::cerr << "    Parsing output\n";
            std::string line{};
            {
                FATAL_ERROR_IF(!std::getline(output, line), "Could parse first line of output of ", benchmarkInstance->path);
                FATAL_ERROR_IF(cutLeadingSpaces(line) != 0, "First line of output of ", benchmarkInstance->path, " started with a space");
                const auto tokens = splitToTwoTokens(line);
                assignAndValidate(benchmark.baseName, tokens.first, " setting base name from benchmark output");
                assignAndValidate(benchmark.description, tokens.second, " setting description from benchmark output");
            }

            // Parse rest of the output - test cases and their arguments
            TestCases::iterator currentTestCase = benchmark.testCases.end();
            while (std::getline(output, line)) {
                switch (cutLeadingSpaces(line)) {
                case 1: {
                    const auto tokens = splitToTwoTokens(line);
                    TestCase &testCase = benchmark.testCases[tokens.first];
                    assignAndValidate(testCase.name, tokens.first, "assigning test case name");
                    assignAndValidate(testCase.help, tokens.second, "assigning test case help message");
                    auto [it, inserted] = testCase.apis.insert(benchmarkInstance->api);
                    FATAL_ERROR_IF(!inserted, "API for test case was duplicated");

                    currentTestCase = benchmark.testCases.find(tokens.first);
                    break;
                }
                case 2: {
                    FATAL_ERROR_IF(currentTestCase == benchmark.testCases.end(), "Argument was printed before any test cases");
                    const auto tokens = splitToTwoTokens(line);
                    TestCaseArgument &argument = currentTestCase->second.arguments[tokens.first];
                    assignAndValidate(argument.name, tokens.first, "assigning argument name");
                    assignAndValidate(argument.help, tokens.second, "assigning argument help message");
                    break;
                }
                default:
                    FATAL_ERROR("Invalid number of leading spaces detected");
                }
            }
        }
    }

    // Generate Markdown code (with embedded HTML)
    std::cerr << "Outputting documentation in Markdown format\n";
    for (const auto &location : locations) {
        FileSystem::path filePath(static_cast<const std::string &>(path));
        if (location.size() > 0) {
            filePath.append(location);
        }
        filePath.append(static_cast<const std::string &>(fileName));
        std::cerr << filePath.string() << ":" << std::endl;

        FileHelper::FileOrConsole outputFileWrapper{filePath.string(), std::ios::out, std::cout};
        std::ostream &outputFile = outputFileWrapper.get();
        for (const auto &entry : benchmarks) {
            const Benchmark &benchmark = entry.second;

            if (std::find(benchmark.locations.begin(), benchmark.locations.end(), location) == benchmark.locations.end()) {
                continue;
            }

            std::vector<Api> apiColumns = getApisForBenchmark(benchmark, location);
            std::vector<std::string> apiNames = {};
            for (Api api : apiColumns) {
                std::string apiName = std::to_string(api);
                std::transform(apiName.begin(), apiName.end(), apiName.begin(), ::toupper);
                apiNames.push_back(apiName);
            }
            std::cerr << "  " << benchmark.baseName << std::endl;

            outputFile << "# " << benchmark.baseName << '\n';
            outputFile << benchmark.description << '\n';
            outputFile << "| Test name | Description | Params |";
            for (const auto &apiName : apiNames) {
                outputFile << ' ' << apiName << " |";
            }
            outputFile << '\n';

            outputFile << "|-----------|-------------|--------|";
            for (const auto &apiName : apiNames) {
                const size_t dashCount = apiName.size() + 2;
                outputFile << std::string(dashCount, '-') << '|';
            }
            outputFile << '\n';
            for (const auto &entry : benchmark.testCases) {
                const TestCase &testCase = entry.second;

                if (location.empty() && !hasAnyApi(testCase, apiColumns)) {
                    continue;
                }

                outputFile << testCase.name << '|';
                outputFile << testCase.help << '|';
                outputFile << "<ul>";
                for (const auto &entry : testCase.arguments) {
                    const TestCaseArgument &argument = entry.second;
                    outputFile << "<li>--" << argument.name << " " << argument.help << "</li>";
                }

                outputFile << "</ul>|";
                for (Api api : apiColumns) {
                    outputFile << (testCase.apis.find(api) != testCase.apis.end() ? ":heavy_check_mark:" : ":x:") << '|';
                }
                outputFile << '\n';
            }
            outputFile << "\n\n\n";
        }
    }

    std::cerr << "Done\n";

    return 0;
}
