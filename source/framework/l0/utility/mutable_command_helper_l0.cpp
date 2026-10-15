/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "mutable_command_helper_l0.h"

namespace L0::MutableCommandHelper {
namespace {
ze_mutable_command_exp_flags_t toSingleMutableCommandFlag(MutableCommandFlag flag) {
    switch (flag) {
    case MutableCommandFlag::KernelArguments:
        return ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_ARGUMENTS;
    case MutableCommandFlag::GroupCount:
        return ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_COUNT;
    case MutableCommandFlag::GroupSize:
        return ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_SIZE;
    case MutableCommandFlag::GlobalOffset:
        return ZE_MUTABLE_COMMAND_EXP_FLAG_GLOBAL_OFFSET;
    case MutableCommandFlag::SignalEvent:
        return ZE_MUTABLE_COMMAND_EXP_FLAG_SIGNAL_EVENT;
    case MutableCommandFlag::WaitEvents:
        return ZE_MUTABLE_COMMAND_EXP_FLAG_WAIT_EVENTS;
    case MutableCommandFlag::KernelInstruction:
        return ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_INSTRUCTION;
    case MutableCommandFlag::GraphArguments:
        return ZE_MUTABLE_COMMAND_EXP_FLAG_GRAPH_ARGUMENTS;
    default:
        return 0;
    }
}
} // namespace

ze_mutable_command_exp_flags_t toMutableCommandFlags(MutableCommandFlag flag, ze_mutable_command_exp_flags_t supportedFlags) {
    if (flag == MutableCommandFlag::All) {
        return supportedFlags;
    }
    return toSingleMutableCommandFlag(flag) & supportedFlags;
}
} // namespace L0::MutableCommandHelper
