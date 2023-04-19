#!/bin/env bash

#
# Copyright (C) 2022-2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

# substitute first argument with icpx
# if the command line contains -fsycl,
# otherwise run as is

set -e

if [[ " $@ " =~ .*\ -fsycl\ .* ]]; then
    icpx "${@:2}"
else
    "$@"
fi