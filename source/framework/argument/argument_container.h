/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/enum/api.h"
#include "framework/utility/command_line_argument.h"

#include <string>
#include <vector>

struct Argument;

struct ArgumentContainer {
    void pushArgument(Argument &argument);
    const std::vector<Argument *> &getArguments() const;

    bool parseArgument(CommandLineArgument &commandLineArgument);
    bool parseArguments(CommandLineArguments &commandLineArguments);
    virtual bool validateArguments() const;
    std::string getHelp(size_t indent) const;
    std::vector<const Argument *> getUnparsedArguments() const;

    virtual ~ArgumentContainer() = default;

  protected:
    std::vector<Argument *> arguments;
    virtual bool validateArgumentsExtra() const { return true; } // This is optional. Use this for validating dependencies between arguments if any.
};
