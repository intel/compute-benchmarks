/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/argument_container.h"
#include "framework/argument/basic_argument.h"
#include "framework/argument/boolean_flag_argument.h"
#include "framework/argument/enum/api_argument.h"
#include "framework/argument/enum/device_selection_argument.h"
#include "framework/argument/enum/profiler_type_argument.h"
#include "framework/argument/string_argument.h"
#include "framework/argument/string_list_argument.h"
#include "framework/utility/command_line_argument.h"

#include <memory>

struct Configuration : ArgumentContainer {
  private:
    static std::unique_ptr<Configuration> instance;

  public:
    Configuration();

    enum class PrintType {
        Default,
        DefaultWithVerbose,
        Csv,
        Noop
    } printType = PrintType::Default;

    static bool parseArgumentsForConfiguration(CommandLineArguments &arguments);
    static void loadDefaultConfiguration();
    static Configuration &get();

    bool validateArgumentsExtra() const override;

    // Diagnostic params
    BooleanFlagArgument help;
    BooleanFlagArgument version;
    BooleanFlagArgument hwInfo;
    BooleanFlagArgument generateDocs;

    // OCL params
    IntegerArgument oclPlatformIndex;
    NonNegativeIntegerArgument oclDeviceIndex;
    BooleanArgument useOOQ;

    // L0 params
    NonNegativeIntegerArgument l0DriverIndex;
    NonNegativeIntegerArgument l0DeviceIndex;

    // UR params
    NonNegativeIntegerArgument urPlatformIndex;
    NonNegativeIntegerArgument urDeviceIndex;

    // Api agnostic params
    StringArgument test;
    DeviceSelectionArgument subDeviceSelection;
    BooleanFlagArgument csv;
    BooleanFlagArgument verbose;
    BooleanFlagArgument interactivePrints;
    PositiveIntegerArgument iterations;
    IntegerArgument sleepFor;
    ApiArgument selectedApi;
    BooleanFlagArgument noIntelExtensions;
    BooleanFlagArgument dumpCommandLines;
    BooleanFlagArgument allowLimitedTests;
    BooleanFlagArgument noop;
    BooleanFlagArgument noHeaders;
    BooleanFlagArgument noColumnNames;
    BooleanFlagArgument doNotPrintBandwidth;
    BooleanArgument dumpErrorsImmediately;
    StringListArgument argFilter;
    StringListArgument testFilter;
    BooleanFlagArgument returnSubmissionTimeInsteadOfWorkloadTime;
    BooleanFlagArgument markTimers;
    BooleanFlagArgument measurePower;
    BooleanFlagArgument printAllResults;
    ProfilerTypeArgument profilerType;

    // Test specific params
    BooleanFlagArgument extended;
    BooleanFlagArgument reducedSizeCAL;
};

inline bool isNoopRun() {
    return Configuration::get().noop;
}
