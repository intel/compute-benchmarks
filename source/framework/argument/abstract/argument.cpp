/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "argument.h"

#include "framework/argument/argument_container.h"
#include "framework/utility/command_line_argument.h"

#include <iomanip>

Argument::Argument(ArgumentContainer &parent, const std::string &key, const std::string &extraHelp)
    : key(key),
      extraHelp(extraHelp) {
    parent.pushArgument(*this);
}

const std::string Argument::getKey() const {
    return key;
}

std::string Argument::getHelp() const {
    std::ostringstream result;
    if (!extraHelp.empty()) {
        const int maxSize = 25;
        result << std::left << std::setw(maxSize);
    }

    result << getHelpEntry(key);
    if (!extraHelp.empty()) {
        result << extraHelp;
    }
    return result.str();
}

std::string Argument::getExtraHelp() const {
    return extraHelp;
}

void Argument::parse(CommandLineArgument &argument) {
    if (argument.isKeyEqualTo(this->key)) {
        argument.markAsProcessed();
        parseImpl(argument.getValue());
        markAsParsed();
    }
}

void Argument::markAsParsed() {
    this->parsed = true;
}

std::string Argument::getHelpEntry(const std::string &argumentKey) const {
    return std::string("--") + argumentKey + "=X ";
}
