/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/utility/error.h"
#include "framework/utility/linux/error.h"
#include "framework/utility/process.h"
#include "framework/utility/process_synchronization_helper.h"

#include <fcntl.h>
#include <memory>
#include <sstream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

struct ProcessDataLinux {
    struct ProcessPipes {
        int pipes[2] = {};
        const int &read = pipes[0];
        const int &write = pipes[1];
    };
    ProcessPipes synchronizationPipeParentToChild = {};
    ProcessPipes synchronizationPipeChildToParent = {};
    ProcessPipes measurementPipe = {};
    ProcessPipes stdOutPipe = {};

    pid_t childPid = {};
    bool ended = false;
    TestResult result = TestResult::Error;
    bool hasStdOut = false;
    std::string stdOut = {};
    bool hasMeasurements = false;
    std::string measurements = {};
};

void Process::run() {
    auto processDataLinux = std::make_unique<ProcessDataLinux>();

    // Create pipes for stdout and stdin of the child process.
    FATAL_ERROR_IF_SYS_CALL_FAILED(pipe(processDataLinux->synchronizationPipeParentToChild.pipes), "Creating pipe failed, ");
    FATAL_ERROR_IF_SYS_CALL_FAILED(pipe(processDataLinux->synchronizationPipeChildToParent.pipes), "Creating pipe failed, ");
    FATAL_ERROR_IF_SYS_CALL_FAILED(pipe(processDataLinux->measurementPipe.pipes), "Creating pipe failed, ");
    FATAL_ERROR_IF_SYS_CALL_FAILED(pipe(processDataLinux->stdOutPipe.pipes), "Creating pipe failed, ");

    // Fork the process
    processDataLinux->childPid = fork();
    FATAL_ERROR_IF(processDataLinux->childPid == -1, "Creating process failed");

    // Different behaviour for parent and slave process
    if (processDataLinux->childPid != 0) {
        // We're in parent process

        // Close pipes that we won't need (these are descriptors, which will be used by child)
        FATAL_ERROR_IF_SYS_CALL_FAILED(close(processDataLinux->synchronizationPipeParentToChild.read), "closing pipe failed");
        FATAL_ERROR_IF_SYS_CALL_FAILED(close(processDataLinux->synchronizationPipeChildToParent.write), "closing pipe failed");
        FATAL_ERROR_IF_SYS_CALL_FAILED(close(processDataLinux->measurementPipe.write), "closing pipe failed");
        FATAL_ERROR_IF_SYS_CALL_FAILED(close(processDataLinux->stdOutPipe.write), "closing pipe failed");

        // Store all data in Process class
        this->osSpecificData = processDataLinux.release();
    } else {
        // We're in child process

        // Redirect stdout to our pipe
        FATAL_ERROR_IF_SYS_CALL_FAILED(dup2(processDataLinux->stdOutPipe.write, STDOUT_FILENO), "dup2 for stdout failed");

        // Close pipes that we won't need (these are descriptors, which will be used by parent)
        FATAL_ERROR_IF_SYS_CALL_FAILED(close(processDataLinux->synchronizationPipeParentToChild.write), "closing pipe failed");
        FATAL_ERROR_IF_SYS_CALL_FAILED(close(processDataLinux->synchronizationPipeChildToParent.read), "closing pipe failed");
        FATAL_ERROR_IF_SYS_CALL_FAILED(close(processDataLinux->measurementPipe.read), "closing pipe failed");
        FATAL_ERROR_IF_SYS_CALL_FAILED(close(processDataLinux->stdOutPipe.read), "closing pipe failed");

        // Below pipe endpoints will be explicitly used by the child workload and they should be closed by it.
        this->addArgument("synchronizationPipeIn", std::to_string(processDataLinux->synchronizationPipeParentToChild.read));
        this->addArgument("synchronizationPipeOut", std::to_string(processDataLinux->synchronizationPipeChildToParent.write));
        this->addArgument("measurementPipe", std::to_string(processDataLinux->measurementPipe.write));

        // Prepare arguments
        std::vector<std::string> argumentsForExecStrings = {};
        argumentsForExecStrings.reserve(this->arguments.size());
        for (auto &argument : this->arguments) {
            std::string str = argument.first;
            if (!argument.second.empty()) {
                str += "=";
                str += argument.second;
            }
            argumentsForExecStrings.push_back(std::move(str));
        }
        std::vector<char *> argumentsForExec = {};
        argumentsForExec.reserve(argumentsForExec.size() + 1);
        for (auto &argumentsForExecString : argumentsForExecStrings) {
            argumentsForExec.push_back(argumentsForExecString.data());
        }
        argumentsForExec.push_back(nullptr);

        // Prepare environment
        for (auto &envVariable : this->envVariables) {
            FATAL_ERROR_IF_SYS_CALL_FAILED(setenv(envVariable.first.c_str(), envVariable.second.c_str(), 1), "setenv failed");
        }

        // Enable inheritance for requested handles
        for (int handle : handlesForInheritance) {
            int currentFlags = fcntl(handle, F_GETFD);
            FATAL_ERROR_IF_SYS_CALL_FAILED(currentFlags, "Failed getting descriptor flags for fd=", handle)
            FATAL_ERROR_IF_SYS_CALL_FAILED(fcntl(handle, F_SETFD, currentFlags & ~FD_CLOEXEC), "Failed getting descriptor flags for fd=", handle);
        }

        // Load new binary image
        extern char **environ;
        const int execResult = execve(this->exeName.c_str(), argumentsForExec.data(), environ);
        FATAL_ERROR_IF_SYS_CALL_FAILED(execResult, "Sys call execve failed, ");
        FATAL_ERROR("Unreachable code after execve");
    }
}

void Process::freeOsSpecificData() {
    ProcessDataLinux *processDataLinux = static_cast<ProcessDataLinux *>(this->osSpecificData);
    if (processDataLinux == nullptr) {
        return;
    }

    // Close pipes, that were used by the parent
    FATAL_ERROR_IF_SYS_CALL_FAILED(close(processDataLinux->synchronizationPipeParentToChild.write), "closing pipe failed");
    FATAL_ERROR_IF_SYS_CALL_FAILED(close(processDataLinux->synchronizationPipeChildToParent.read), "closing pipe failed");
    FATAL_ERROR_IF_SYS_CALL_FAILED(close(processDataLinux->measurementPipe.read), "closing pipe failed");

    delete processDataLinux;
}

void Process::waitForFinish() {
    ProcessDataLinux *processDataLinux = static_cast<ProcessDataLinux *>(this->osSpecificData);
    if (processDataLinux->ended) {
        return;
    }

    while (true) {
        int status{};
        int pid = waitpid(processDataLinux->childPid, &status, 0);
        FATAL_ERROR_IF(pid == -1, std::string("waitpid() returned an error, ") + getErrorFromErrno());
        FATAL_ERROR_IF(pid != processDataLinux->childPid, "waitpid() signalled from wrong child process");
        FATAL_ERROR_IF(WIFSIGNALED(status), "child process killed by signal")
        FATAL_ERROR_IF(WIFSTOPPED(status), "child process stopped by signal")

        if (WIFEXITED(status)) {
            processDataLinux->result = static_cast<TestResult>(WEXITSTATUS(status));
            break;
        }
    }

    processDataLinux->ended = true;
}

TestResult Process::getResult() {
    waitForFinish();
    ProcessDataLinux *processDataLinux = static_cast<ProcessDataLinux *>(this->osSpecificData);
    return processDataLinux->result;
}

const static std::string readEntirePipe(ProcessDataLinux::ProcessPipes processPipes) {
    std::ostringstream output{};
    const static size_t bufferSize = 1024u;
    char buffer[bufferSize];
    while (true) {
        ssize_t numberOfBytesRead = read(processPipes.read, buffer, bufferSize);
        FATAL_ERROR_IF_SYS_CALL_FAILED(numberOfBytesRead, "reading a child process pipe failed");

        if (numberOfBytesRead == 0) {
            break;
        }

        output << std::string{buffer, static_cast<size_t>(numberOfBytesRead)};
    }

    return output.str();
}

const std::string &Process::getStdout() {
    ProcessDataLinux *processDataLinux = static_cast<ProcessDataLinux *>(this->osSpecificData);

    if (!processDataLinux->hasStdOut) {
        waitForFinish();
        processDataLinux->stdOut = readEntirePipe(processDataLinux->stdOutPipe);
        processDataLinux->hasStdOut = true;
    }

    return processDataLinux->stdOut;
}

const std::string &Process::getMeasurements() {
    ProcessDataLinux *processDataLinux = static_cast<ProcessDataLinux *>(this->osSpecificData);

    if (!processDataLinux->hasMeasurements) {
        waitForFinish();
        processDataLinux->measurements = readEntirePipe(processDataLinux->measurementPipe);
        processDataLinux->hasMeasurements = true;
    }

    return processDataLinux->measurements;
}

void Process::synchronizationSignal() {
    ProcessDataLinux *processDataLinux = static_cast<ProcessDataLinux *>(this->osSpecificData);

    char buffer = ProcessSynchronizationHelper::synchronizationChar;
    ssize_t numberOfBytesWritten = write(processDataLinux->synchronizationPipeParentToChild.write, &buffer, 1);
    FATAL_ERROR_IF_SYS_CALL_FAILED(numberOfBytesWritten, "reading a child process stdOut failed");
    FATAL_ERROR_IF(numberOfBytesWritten == 0, "No character was written when signalling child process");
}

void Process::synchronizationWait() {
    ProcessDataLinux *processDataLinux = static_cast<ProcessDataLinux *>(this->osSpecificData);

    char buffer = {};
    ssize_t numberOfBytesRead = read(processDataLinux->synchronizationPipeChildToParent.read, &buffer, 1u);
    FATAL_ERROR_IF_SYS_CALL_FAILED(numberOfBytesRead, "reading a child process stdOut failed");
    FATAL_ERROR_IF(numberOfBytesRead == 0, "No character was read when waiting on a child process");
    FATAL_ERROR_IF(buffer != ProcessSynchronizationHelper::synchronizationChar, std::string("Invalid synchronization received from child process: '") + buffer + "'");
}
