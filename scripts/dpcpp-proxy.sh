#!/bin/env bash

#
# Copyright (C) 2022 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

# substitute first argument with dpcpp
# if the command line contains -fsycl,
# otherwise run as is

set -e

if [[ " $@ " =~ .*\ -fsycl\ .* ]]; then
    dpcpp "${@:2}"
else
    "$@"
fi