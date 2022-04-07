#
# Copyright (C) 2022 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

if [ `dpkg --list | grep " clang-format " | wc -l` -eq 0 ]; then
    sudo apt-get install clang-format
fi

root_dir=`echo ${BASH_SOURCE[0]} | xargs realpath | xargs dirname | xargs dirname `
find "$root_dir/source" | grep -E "*\.(cpp|inl|h|c|hpp)$" | xargs clang-format -i --verbose
