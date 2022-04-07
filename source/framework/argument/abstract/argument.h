/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/utility/error.h"

#include <sstream>
#include <string>

struct ArgumentContainer;
class CommandLineArgument;

struct Argument {
    Argument(ArgumentContainer &parent, const std::string &key) : Argument(parent, key, "") {}
    Argument(ArgumentContainer &parent, const std::string &key, const std::string &extraHelp);

    const std::string getKey() const;
    std::string getHelp() const;
    std::string getExtraHelp() const;

    virtual std::string toString() const {
        std::ostringstream result;
        result << key << "=" << toStringValue();
        return result.str();
    }

    void parse(CommandLineArgument &argument);
    void markAsParsed();

    virtual bool validate() const {
        return true;
    }

    bool wasParsed() const {
        return parsed;
    }

  protected:
    virtual void parseImpl(const std::string &value) = 0;
    virtual std::string toStringValue() const = 0;
    virtual std::string getHelpEntry(const std::string &key) const;

  private:
    const std::string key;
    const std::string extraHelp;
    bool parsed = false;
};
