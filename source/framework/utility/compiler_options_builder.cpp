/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "compiler_options_builder.h"

#include "framework/utility/string_utils.h"

void CompilerOptionsBuilder::addOption(const char *option) {
    options << option << ' ';
}

void CompilerOptionsBuilder::addOptionOpenCl30() {
    addOption("-cl-std=CL3.0");
}

void CompilerOptionsBuilder::addDefinition(const char *key) {
    options << "-D" << key << ' ';
}

void CompilerOptionsBuilder::addDefinitionKeyValue(const char *key, const char *value) {
    options << "-D" << key << '=' << value << ' ';
}

void CompilerOptionsBuilder::addDefinitionKeyValue(const char *key, const std::string &value) {
    addDefinitionKeyValue(key, value.c_str());
}

void CompilerOptionsBuilder::addDefinitionKeyValue(const char *key, size_t value) {
    options << "-D" << key << '=' << value << ' ';
}

void CompilerOptionsBuilder::addMacro(const char *name, const std::vector<const char *> &arguments, const char *body) {
    const static auto toString = +[](const char *a) { return std::string(a); };
    options << "-D" << name
            << '(' << joinStrings(",", arguments, toString) << ')'
            << '='
            << body << ' ';
}

std::string CompilerOptionsBuilder::str() const {
    return options.str();
}
