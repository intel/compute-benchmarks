/*
 * Copyright (C) 2023-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/utility/linux/ipc.h"

#include "framework/utility/linux/error.h"

#include <memory>

#ifdef USE_PIDFD
#include <sys/syscall.h>
#endif // USE_PIDFD

TestResult socketCreate(int &socketNew) {
    socketNew = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socketNew <= 0) {
        return TestResult::Error;
    }
    const int socketOpt = 1;
    if (0 != setsockopt(socketNew, SOL_SOCKET, SO_REUSEADDR, &socketOpt, sizeof(socketOpt))) {
        return TestResult::Error;
    }
    return TestResult::Success;
}

TestResult socketBindAndListen(const int socketLocal, const std::string &socketName) {
    if (socketName.length() + 1 > sizeof(sockaddr_un::sun_path)) {
        return TestResult::Error;
    }
    sockaddr_un address{};
    address.sun_family = AF_UNIX;
    for (size_t i = 0; i < sizeof(sockaddr_un::sun_path); i++) {
        if (i < socketName.length()) {
            address.sun_path[i] = socketName[i];
        } else {
            address.sun_path[i] = '\0';
        }
    }

    unlink(socketName.c_str());

    if (0 != bind(socketLocal, reinterpret_cast<sockaddr *>(&address), sizeof(address))) {
        return TestResult::Error;
    }
    if (0 != listen(socketLocal, 1024)) {
        return TestResult::Error;
    }
    return TestResult::Success;
}

TestResult socketAccept(const int socketListening, std::vector<int> &activeSockets, int &socketNew) {
    fd_set fdSet{};
    FD_ZERO(&fdSet);
    FD_SET(socketListening, &fdSet);
    int maxFd = socketListening;
    for (int socketDescriptor : activeSockets) {
        if (socketDescriptor > 0) {
            FD_SET(socketDescriptor, &fdSet);
        }
        if (socketDescriptor > maxFd) {
            maxFd = socketDescriptor;
        }
    }
    const int activity = select(maxFd + 1, &fdSet, nullptr, nullptr, nullptr);
    if ((activity < 0) && (errno != EINTR)) {
        std::cerr << "Server select() failed with " << getErrorFromErrno() << std::endl;
        return TestResult::Error;
    }
    if (!FD_ISSET(socketListening, &fdSet)) {
        return TestResult::Error;
    }

    sockaddr_un address;
    socklen_t addressLen = sizeof(address);
    socketNew = accept(socketListening, reinterpret_cast<sockaddr *>(&address), &addressLen);
    if (socketNew <= 0) {
        return TestResult::Error;
    }

    activeSockets.push_back(socketNew);

    return TestResult::Success;
}

TestResult socketConnect(const int socketLocal, const std::string &socketName) {
    if (socketName.length() + 1 > sizeof(sockaddr_un::sun_path)) {
        return TestResult::Error;
    }
    sockaddr_un address{};
    address.sun_family = AF_UNIX;
    for (size_t i = 0; i < sizeof(sockaddr_un::sun_path); i++) {
        if (i < socketName.length()) {
            address.sun_path[i] = socketName[i];
        } else {
            address.sun_path[i] = '\0';
        }
    }

    while (-1 == connect(socketLocal, reinterpret_cast<sockaddr *>(&address), sizeof(address))) {
        if (errno == ENOENT) {
            usleep(10000);
        } else {
            std::cerr << "Worker connect() failed with " << getErrorFromErrno() << std::endl;
            return TestResult::Error;
        }
    }

    return TestResult::Success;
}

TestResult socketSendDataWithFd(const int socketReceiver, const int fd, void *data, const ssize_t nBytes) {
    std::unique_ptr<uint8_t[]> controlMessage(new uint8_t[CMSG_SPACE(nBytes)]());

    iovec message{};
    message.iov_base = data;
    message.iov_len = nBytes;

    msghdr messageHeader{};
    messageHeader.msg_iov = &message;
    messageHeader.msg_iovlen = 1;
    messageHeader.msg_control = controlMessage.get();
    messageHeader.msg_controllen = CMSG_LEN(sizeof(int));

    cmsghdr *controlMessageHeaderPtr = CMSG_FIRSTHDR(&messageHeader);
    if (nullptr == controlMessageHeaderPtr) {
        return TestResult::Error;
    }
    controlMessageHeaderPtr->cmsg_type = SCM_RIGHTS;
    controlMessageHeaderPtr->cmsg_level = SOL_SOCKET;
    controlMessageHeaderPtr->cmsg_len = CMSG_LEN(sizeof(int));
    *reinterpret_cast<int *>(CMSG_DATA(controlMessageHeaderPtr)) = fd;

    if (nBytes != sendmsg(socketReceiver, &messageHeader, 0)) {
        return TestResult::Error;
    }

    return TestResult::Success;
}

TestResult socketRecvDataWithFd(const int socketSender, int &fd, void *data, const ssize_t nBytes) {
    std::unique_ptr<uint8_t[]> controlMessage(new uint8_t[CMSG_SPACE(nBytes)]());

    iovec message{};
    message.iov_base = data;
    message.iov_len = nBytes;

    msghdr messageHeader{};
    messageHeader.msg_iov = &message;
    messageHeader.msg_iovlen = 1;
    messageHeader.msg_control = controlMessage.get();
    messageHeader.msg_controllen = CMSG_LEN(sizeof(int));

    if (nBytes != recvmsg(socketSender, &messageHeader, 0)) {
        return TestResult::Error;
    }

    cmsghdr *controlMessageHeaderPtr = CMSG_FIRSTHDR(&messageHeader);
    if (nullptr == controlMessageHeaderPtr) {
        return TestResult::Error;
    }
    fd = *reinterpret_cast<int *>(CMSG_DATA(controlMessageHeaderPtr));
    if (fd <= 0) {
        return TestResult::Error;
    }

    return TestResult::Success;
}

#ifdef USE_PIDFD

std::string serializeBinaryToStr(void *data, const size_t nBytes) {
    std::stringstream stream;
    stream << std::hex << std::setfill('0');
    for (size_t i = 0; i < nBytes; i++) {
        stream << std::setw(2) << static_cast<int>(static_cast<uint8_t *>(data)[i]);
    }
    return stream.str();
}

void deserializeStrToBinary(void *data, const std::string str) {
    for (size_t i = 0; i < str.length() / 2; i++) {
        reinterpret_cast<uint8_t *>(data)[i] = std::stoi(str.substr(2 * i, 2), nullptr, 16);
    }
}

TestResult translateParentFd(const pid_t parentPid, const int parentFd, int &fd) {
    const int parentPidFd = syscall(SYS_pidfd_open, parentPid, 0);
    if (parentPidFd <= 0) {
        return TestResult::Error;
    }
    fd = syscall(SYS_pidfd_getfd, parentPidFd, parentFd, 0);
    if (fd <= 0) {
        return TestResult::Error;
    }

    return TestResult::Success;
}

#endif // USE_PIDFD
