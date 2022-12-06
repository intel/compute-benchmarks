/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/configuration.h"

#include "framework/benchmark_info.h"

std::unique_ptr<Configuration> Configuration::instance = {};

Configuration::Configuration()
    : help(*this, "help", "Shows this message"),
      version(*this, "version", "Shows benchmark version"),
      hwInfo(*this, "hwInfo", "Shows available devices"),
      generateDocs(*this, "generateDocs", "Generate .md file describing available tests"),
      oclPlatformIndex(*this, "oclPlatformIndex", "OpenCL platform index"),
      oclDeviceIndex(*this, "oclDeviceIndex", "OpenCL device index inside the platform"),
      oclUseOOQ(*this, "oclUseOOQ", "Use out of order queue if it is supported"),
      l0DriverIndex(*this, "l0DriverIndex", "LevelZero driver index"),
      l0DeviceIndex(*this, "l0DeviceIndex", "LevelZero device index inside the driver"),
      test(*this, "test", "Selects particular test for execution. All arguments of the test must be provided"),
      subDeviceSelection(*this, "subDeviceSelection", "Device to be used in the benchmarks. Might be ignored by some specific tests"),
      csv(*this, "csv", "dump results in CSV format for easy imports to spreadsheets"),
      verbose(*this, "verbose", "dump results from all iterations"),
      interactivePrints(*this, "interactivePrints", "display test name before running it. May cause unexcpected results when redirecting output to files."),
      iterations(*this, "iterations", "select how many times each test will be run"),
      selectedApi(*this, "api", "Compute API to be used"),
      noIntelExtensions(*this, "no-intel-extensions", "do not run benchmark requiring Intel specific extensions"),
      dumpCommandLines(*this, "dumpCommandLines", "output commandline arguments to run the each test"),
      noop(*this, "noop", "do not run any tests, only print their names and parameters"),
      noHeaders(*this, "noHeaders", "Do not print any informational messages at the top of the output"),
      noColumnNames(*this, "noColumnNames", "Do not print column names at the top of the output"),
      doNotPrintBandwidth(*this, "doNotPrintBandwidth", "Make every results that are normally in [GB/s] to be printed in [us]"),
      dumpErrorsImmediately(*this, "dumpErrorsImmediately", "print errors to stdout immediately after they happen, not at the end of the run"),
      argFilter(*this, "argFilter", "filter tests by their arguments"),
      testFilter(*this, "testFilter", "filter tests by their names"),
      returnSubmissionTimeInsteadOfWorkloadTime(*this, "forceSubmissionProfiling", "Overrides profiling to return submission time instead of workload time"),
      markTimers(*this, "markTimers", "Provides prints around Timer Start & End") {

    // Diagnostic params
    help = false;
    version = false;
    hwInfo = false;
    generateDocs = false;

    // OCL params
    oclPlatformIndex = -1;
    oclDeviceIndex = 0;
    oclUseOOQ = true;

    // L0 params
    l0DriverIndex = 0;
    l0DeviceIndex = 0;

    // Api agnostic params
    test = "";
    csv = false;
    verbose = false;
    interactivePrints = false;
    iterations = 10;
    selectedApi = Api::All;
    noIntelExtensions = false;
    dumpCommandLines = false;
    subDeviceSelection = DeviceSelection::Root;
    noop = false;
    noHeaders = false;
    noColumnNames = false;
    doNotPrintBandwidth = false;
    dumpErrorsImmediately = false;
    argFilter = std::vector<std::string>();
    testFilter = std::vector<std::string>();
    returnSubmissionTimeInsteadOfWorkloadTime = false;
}

bool Configuration::parseArgumentsForConfiguration(CommandLineArguments &arguments) {
    loadDefaultConfiguration();
    auto configuration = Configuration::instance.get();

    for (auto &argument : arguments) {
        if (!configuration->parseArgument(argument)) {
            return false;
        }
    }

    if (!configuration->validateArguments()) {
        return false;
    }

    if (configuration->csv) {
        configuration->printType = Configuration::PrintType::Csv;
    }
    if (configuration->verbose) {
        configuration->printType = Configuration::PrintType::DefaultWithVerbose;
    }
    if (configuration->noop) {
        configuration->printType = Configuration::PrintType::Noop;
    }

    return true;
}

void Configuration::loadDefaultConfiguration() {
    DEVELOPER_WARNING_IF(Configuration::instance != nullptr, "Configuration parsed multiple times");
    Configuration::instance = std::make_unique<Configuration>();
}

Configuration &Configuration::get() {
    FATAL_ERROR_IF(Configuration::instance == nullptr, "Configuration was not parsed");
    return *Configuration::instance;
}

bool Configuration::validateArgumentsExtra() const {
    if (csv && verbose) {
        return false;
    }
    return true;
}
