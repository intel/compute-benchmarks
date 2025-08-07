/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct RecordGraphArguments : TestCaseArgumentContainer {
    IntegerArgument nForksInLvl;
    IntegerArgument nLvls;
    IntegerArgument nCmdSetsInLvl;
    IntegerArgument nInstantiations;
    IntegerArgument nAppendKern;
    IntegerArgument nAppendCopy;
    BooleanArgument mRec;
    BooleanArgument mInst;
    BooleanArgument mDest;
    BooleanArgument emulate;

    RecordGraphArguments() : nForksInLvl(*this, "nForksInLvl", "Number of forks to introduce in the graph per graph level."),
                             nLvls(*this, "nLvls", "Number of levels (nesting) in the graph using forks."),
                             nCmdSetsInLvl(*this, "nCmdSetsInLvl", "Number of command sets per level."),
                             nInstantiations(*this, "nInstantiations", "Number of executable graphs to instantiate."),
                             nAppendKern(*this, "nAppendKern", "Number of appendLaunchKernel calls per command set."),
                             nAppendCopy(*this, "nAppendCopy", "Number of appendCopy calls per command set"),
                             mRec(*this, "mRec", "Take graph recording phase into account in perf measurment."),
                             mInst(*this, "mInst", "Take graph instantion phase into account in perf measurment."),
                             mDest(*this, "mDest", "Take graph destroy phase into account in perf measurment."),
                             emulate(*this, "emulate", "Emulate record and replay graph API using regular commandlists.") {}
};

struct RecordGraph : TestCase<RecordGraphArguments> {
    using TestCase<RecordGraphArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "RecordGraph";
    }

    std::string getHelp() const override {
        return "measures overhead of recording a graph.";
    }
};
