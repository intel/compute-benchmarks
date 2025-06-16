/*
 * Copyright (C) 2023-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/test_case/test_result.h"

#include <iomanip>
#include <sys/socket.h>
#include <sys/un.h>
#include <vector>

TestResult socketCreate(int &socketNew);

TestResult socketBindAndListen(const int socketLocal, const std::string &socketName);

TestResult socketAccept(const int socketListening, std::vector<int> &activeSockets, int &socketNew);

TestResult socketConnect(const int socketLocal, const std::string &socketName);

TestResult socketSendDataWithFd(const int socketReceiver, const int fd, void *data, const ssize_t nBytes);

TestResult socketRecvDataWithFd(const int socketSender, int &fd, void *data, const ssize_t nBytes);

#ifdef USE_PIDFD

std::string serializeBinaryToStr(void *data, const size_t len);

void deserializeStrToBinary(void *data, const std::string str);

TestResult translateParentFd(const pid_t parentPid, const int parentFd, int &fd);

#endif // USE_PIDFD
