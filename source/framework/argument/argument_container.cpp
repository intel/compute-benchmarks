/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "argument_container.h"

#include "framework/argument/abstract/argument.h"

#include <sstream>

void ArgumentContainer::pushArgument(Argument &argument) {
    this->arguments.push_back(&argument);
}

const std::vector<Argument *> &ArgumentContainer::getArguments() const {
    return arguments;
}

bool ArgumentContainer::parseArgument(CommandLineArgument &commandLineArgument) {
    for (auto &argument : arguments) {
        argument->parse(commandLineArgument);
    }
    return true;
}

bool ArgumentContainer::parseArguments(CommandLineArguments &commandLineArguments) {
    for (CommandLineArgument &commandLineArgument : commandLineArguments) {
        if (!this->parseArgument(commandLineArgument)) {
            return false;
        }
    }
    return true;
}

bool ArgumentContainer::validateArguments() const {
    for (const auto &argument : arguments) {
        if (!argument->validate()) {
            return false;
        }
    }

    if (!validateArgumentsExtra()) {
        return false;
    }

    return true;
}

std::string ArgumentContainer::getHelp(size_t indent) const {
    std::ostringstream result;
    for (const auto &argument : arguments) {
        for (auto i = 0u; i < indent; i++) {
            result << '\t';
        }
        result << argument->getHelp() << '\n';
    }
    return result.str();
}

std::vector<const Argument *> ArgumentContainer::getUnparsedArguments() const {
    std::vector<const Argument *> result = {};
    for (const auto &argument : arguments) {
        if (!argument->wasParsed()) {
            result.push_back(argument);
        }
    }
    return result;
}
