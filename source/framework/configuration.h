/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/argument_container.h"
#include "framework/argument/basic_argument.h"
#include "framework/argument/boolean_flag_argument.h"
#include "framework/argument/cpu_affinity_mask_argument.h"
#include "framework/argument/enum/api_argument.h"
#include "framework/argument/enum/device_selection_argument.h"
#include "framework/argument/enum/profiler_type_argument.h"
#include "framework/argument/string_argument.h"
#include "framework/argument/string_list_argument.h"
#include "framework/utility/command_line_argument.h"

#include "additional_configuration.h"

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

    void applyBenchmarkIterationDefaults();

    bool validateArgumentsExtra() const override;

    BooleanFlagArgument help;
    BooleanFlagArgument version;
    BooleanFlagArgument hwInfo;
    BooleanFlagArgument generateDocs;
    BooleanFlagArgument listTestSuites;

    IntegerArgument oclPlatformIndex;
    NonNegativeIntegerArgument oclDeviceIndex;
    BooleanArgument useOOQ;

    NonNegativeIntegerArgument l0DriverIndex;
    NonNegativeIntegerArgument l0DeviceIndex;

    NonNegativeIntegerArgument urPlatformIndex;
    NonNegativeIntegerArgument urDeviceIndex;

    NonNegativeIntegerArgument vkDeviceIndex;
    BooleanFlagArgument vkEnableValidation;

    NonNegativeIntegerArgument olDeviceIndex;

    StringArgument test;
    DeviceSelectionArgument subDeviceSelection;
    BooleanFlagArgument csv;
    BooleanFlagArgument verbose;
    BooleanFlagArgument interactivePrints;
    PositiveIntegerArgument iterations;
    NonNegativeIntegerArgument warmupIterations;
    NonNegativeIntegerArgument trimOutliers;
    IntegerArgument sleepFor;
    CpuAffinityMaskArgument cpuAffinityMask;
    ApiArgument selectedApi;
    BooleanFlagArgument noIntelExtensions;
    BooleanFlagArgument dumpCommandLines;
    BooleanFlagArgument allowLimitedTests;
    BooleanFlagArgument noop;
    BooleanFlagArgument noHeaders;
    BooleanFlagArgument noColumnNames;
    BooleanFlagArgument noProgressBar;
    StringArgument htmlOutput;
    StringArgument mdOutput;
    BooleanFlagArgument doNotPrintBandwidth;
    BooleanArgument dumpErrorsImmediately;
    StringListArgument argFilter;
    StringListArgument testFilter;
    BooleanFlagArgument returnSubmissionTimeInsteadOfWorkloadTime;
    BooleanFlagArgument markTimers;
    BooleanFlagArgument measurePower;
    BooleanFlagArgument verify;
    BooleanFlagArgument printAllResults;
    BooleanFlagArgument printHistogram;
    ProfilerTypeArgument profilerType;

    BooleanFlagArgument extended;
    BooleanFlagArgument reducedSizeCAL;

    AdditionalConfiguration additionalConfiguration;
};

inline bool isNoopRun() {
    return Configuration::get().noop;
}
