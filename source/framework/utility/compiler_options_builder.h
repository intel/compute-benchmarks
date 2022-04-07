/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <sstream>
#include <vector>

class CompilerOptionsBuilder {
  public:
    void addOption(const char *option);
    void addOptionOpenCl20();

    void addDefinition(const char *key);
    void addDefinitionKeyValue(const char *key, const char *value);
    void addDefinitionKeyValue(const char *key, const std::string &value);
    void addDefinitionKeyValue(const char *key, size_t value);

    void addMacro(const char *name, const std::vector<const char *> &arguments, const char *body);

    std::string str() const;

  private:
    std::ostringstream options;
};
