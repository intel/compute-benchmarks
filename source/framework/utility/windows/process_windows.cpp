/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/utility/error.h"
#include "framework/utility/process.h"
#include "framework/utility/process_synchronization_helper.h"
#include "framework/utility/string_utils.h"
#include "framework/utility/windows/windows.h"

#include <cstdint>
#include <sstream>
#include <thread>

struct ProcessDataWindows {
    // Resources to be freed
    PROCESS_INFORMATION processInfo{};
    struct ProcessPipes {
        HANDLE write = INVALID_HANDLE_VALUE;
        HANDLE read = INVALID_HANDLE_VALUE;
    };
    ProcessPipes processStdOut = {};
    ProcessPipes processStdIn = {};
    ProcessPipes synchronizationPipeParentToChild = {};
    ProcessPipes synchronizationPipeChildToParent = {};
    ProcessPipes measurementPipe = {};
    std::unique_ptr<std::thread> asyncReadThread;

    // Cached Values
    bool ended = false;
    bool hasResult = false;
    TestResult result = TestResult::Error;
    std::string stdOut = {};
    bool hasMeasurements = false;
    std::string measurements = {};
};

class EnvironmentRestorer {
  public:
    EnvironmentRestorer() {
        const char *environment = GetEnvironmentStringsA();
        size_t environmentSize = 0;
        while (environment[environmentSize] != '\0' || environment[environmentSize - 1] != '\0') {
            environmentSize++;
        }
        environmentSize++;

        this->storedEnv = std::make_unique<char[]>(environmentSize);
        std::memcpy(this->storedEnv.get(), environment, environmentSize);
        this->storedEnvSize = environmentSize;
    }

    ~EnvironmentRestorer() {
        if (SetEnvironmentStringsA(this->storedEnv.get()) == 0) {
            FATAL_ERROR_IN_DESTRUCTOR(std::string("restoring environemnt variables, ") + getErrorFromLastErrorCode());
        }
    }

  private:
    std::unique_ptr<char[]> storedEnv = {};
    size_t storedEnvSize = {};
};

// Measurements are written by the child in a single call, so the pipe has to be able to buffer all of
// them. Otherwise the child blocks in WriteFile and never reaches exit, while the parent waits for it.
constexpr static DWORD measurementPipeBufferSize = 1024u * 1024u;

static std::string handleToArgument(HANDLE handle) {
    return std::to_string(reinterpret_cast<uintptr_t>(handle));
}

static const std::string readEntirePipe(HANDLE pipe) {
    std::ostringstream output{};
    const static size_t bufferSize = 1024u;
    CHAR buffer[bufferSize];
    DWORD numberOfBytesRead = {};

    while (true) {
        if (ReadFile(pipe, buffer, bufferSize, &numberOfBytesRead, NULL) == 0) {
            break;
        }
        if (numberOfBytesRead == 0) {
            break;
        }

        output << std::string{buffer, numberOfBytesRead};
    }

    return output.str();
}

void asyncReadThreadBody(ProcessDataWindows *processDataWindows) {
    std::ostringstream stdOutStream{};

    const static size_t bufferSize = 1024u;
    CHAR buffer[bufferSize];
    DWORD numberOfBytesRead = {};

    while (true) {
        BOOL retVal = ReadFile(processDataWindows->processStdOut.read, buffer, bufferSize, &numberOfBytesRead, NULL);

        if (retVal == 0) {
            break;
        }
        if (numberOfBytesRead == 0) {
            break;
        }

        stdOutStream << std::string{buffer, numberOfBytesRead};
    }

    processDataWindows->stdOut = stdOutStream.str();
}

void Process::run() {
    auto processDataWindows = std::make_unique<ProcessDataWindows>();

    // Create pipes for stdout and stdin of the child process. Endpoints that are passed to the process
    // (stdOut.write, processStdIn.read) are closed by the parent right after it is spawned.
    SECURITY_ATTRIBUTES pipeSecutrityAttributes;
    pipeSecutrityAttributes.nLength = sizeof(pipeSecutrityAttributes);
    pipeSecutrityAttributes.bInheritHandle = TRUE;
    pipeSecutrityAttributes.lpSecurityDescriptor = NULL;
    FATAL_ERROR_IF_SYS_CALL_FAILED(CreatePipe(&processDataWindows->processStdOut.read, &processDataWindows->processStdOut.write, &pipeSecutrityAttributes, 0), "creating pipe for stdout")
    FATAL_ERROR_IF_SYS_CALL_FAILED(CreatePipe(&processDataWindows->processStdIn.read, &processDataWindows->processStdIn.write, &pipeSecutrityAttributes, 0), "creating pipe for stdin")
    FATAL_ERROR_IF_SYS_CALL_FAILED(SetHandleInformation(processDataWindows->processStdOut.read, HANDLE_FLAG_INHERIT, 0), "setting handle inheritance")
    FATAL_ERROR_IF_SYS_CALL_FAILED(SetHandleInformation(processDataWindows->processStdIn.write, HANDLE_FLAG_INHERIT, 0), "setting handle inheritance")

    // Dedicated channels for measurements and synchronization, so that anything the child writes to
    // its stdout or stderr (e.g. driver debug output) cannot corrupt them.
    FATAL_ERROR_IF_SYS_CALL_FAILED(CreatePipe(&processDataWindows->synchronizationPipeParentToChild.read, &processDataWindows->synchronizationPipeParentToChild.write, &pipeSecutrityAttributes, 0), "creating synchronization pipe (parent to child)")
    FATAL_ERROR_IF_SYS_CALL_FAILED(CreatePipe(&processDataWindows->synchronizationPipeChildToParent.read, &processDataWindows->synchronizationPipeChildToParent.write, &pipeSecutrityAttributes, 0), "creating synchronization pipe (child to parent)")
    FATAL_ERROR_IF_SYS_CALL_FAILED(CreatePipe(&processDataWindows->measurementPipe.read, &processDataWindows->measurementPipe.write, &pipeSecutrityAttributes, measurementPipeBufferSize), "creating pipe for measurements")
    FATAL_ERROR_IF_SYS_CALL_FAILED(SetHandleInformation(processDataWindows->synchronizationPipeParentToChild.write, HANDLE_FLAG_INHERIT, 0), "setting handle inheritance")
    FATAL_ERROR_IF_SYS_CALL_FAILED(SetHandleInformation(processDataWindows->synchronizationPipeChildToParent.read, HANDLE_FLAG_INHERIT, 0), "setting handle inheritance")
    FATAL_ERROR_IF_SYS_CALL_FAILED(SetHandleInformation(processDataWindows->measurementPipe.read, HANDLE_FLAG_INHERIT, 0), "setting handle inheritance")

    // Prepare arguments
    this->addArgument("synchronizationPipeIn", handleToArgument(processDataWindows->synchronizationPipeParentToChild.read));
    this->addArgument("synchronizationPipeOut", handleToArgument(processDataWindows->synchronizationPipeChildToParent.write));
    this->addArgument("measurementPipe", handleToArgument(processDataWindows->measurementPipe.write));
    std::ostringstream commandLine{};
    for (const auto &argument : this->arguments) {
        commandLine << argument.first;
        if (!argument.second.empty()) {
            commandLine << "=" << argument.second;
        }
        commandLine << ' ';
    }

    // Prepare env variables - create RAII restorer and update existing ones, so the child process inherits them
    EnvironmentRestorer envRestorer = {};
    for (const auto &envVariable : envVariables) {
        FATAL_ERROR_IF_SYS_CALL_FAILED(SetEnvironmentVariableA(envVariable.first.c_str(), envVariable.second.c_str()), "setting env variable");
    }

    // Prepare exeName (.exe extension is Windows-specific)
    std::string exeNameWithExtension = this->exeName;
    if (!endsWith(exeNameWithExtension, ".exe")) {
        exeNameWithExtension += ".exe";
    }

    // Start child process
    STARTUPINFOA startupInfo{};
    startupInfo.cb = sizeof(STARTUPINFO);
    startupInfo.hStdOutput = processDataWindows->processStdOut.write;
    startupInfo.hStdError = processDataWindows->processStdOut.write;
    startupInfo.hStdInput = processDataWindows->processStdIn.read;
    startupInfo.dwFlags |= STARTF_USESTDHANDLES;
    PROCESS_INFORMATION processInfo{};
    FATAL_ERROR_IF_SYS_CALL_FAILED(CreateProcessA(
                                       exeNameWithExtension.c_str(),
                                       commandLine.str().data(),
                                       NULL,
                                       NULL,
                                       TRUE,
                                       0,
                                       NULL,
                                       NULL,
                                       &startupInfo,
                                       &processDataWindows->processInfo),
                                   "creating process");

    // Close the endpoints owned by the child, so that the pipes report end of file once it exits. This has
    // to happen before the next child is spawned, because CreateProcess inherits every inheritable handle.
    FATAL_ERROR_IF_SYS_CALL_FAILED(CloseHandle(processDataWindows->processStdOut.write), "closing stdout.write handle");
    FATAL_ERROR_IF_SYS_CALL_FAILED(CloseHandle(processDataWindows->processStdIn.read), "closing stdin.read handle");
    FATAL_ERROR_IF_SYS_CALL_FAILED(CloseHandle(processDataWindows->synchronizationPipeParentToChild.read), "closing synchronizationPipeParentToChild.read handle");
    FATAL_ERROR_IF_SYS_CALL_FAILED(CloseHandle(processDataWindows->synchronizationPipeChildToParent.write), "closing synchronizationPipeChildToParent.write handle");
    FATAL_ERROR_IF_SYS_CALL_FAILED(CloseHandle(processDataWindows->measurementPipe.write), "closing measurementPipe.write handle");

    // Create an asynchronous thread for reading stdout/stderr pipes. This is needed for cases when
    // process outputs a substantial amount of data exceeding internal system buffer. This causes
    // a deadlock, because WaitForSingleProcess will block inifinitely.
    processDataWindows->asyncReadThread = std::make_unique<std::thread>(asyncReadThreadBody, processDataWindows.get());

    // Set process data
    this->osSpecificData = processDataWindows.release();
}

void Process::freeOsSpecificData() {
    ProcessDataWindows *processDataWindows = static_cast<ProcessDataWindows *>(this->osSpecificData);
    if (processDataWindows == nullptr) {
        return;
    }

    waitForFinish();

    FATAL_ERROR_IF_SYS_CALL_FAILED(CloseHandle(processDataWindows->processInfo.hProcess), "closing process handle");
    FATAL_ERROR_IF_SYS_CALL_FAILED(CloseHandle(processDataWindows->processInfo.hThread), "closing thread handle");
    FATAL_ERROR_IF_SYS_CALL_FAILED(CloseHandle(processDataWindows->processStdOut.read), "closing stdout.read handle");
    FATAL_ERROR_IF_SYS_CALL_FAILED(CloseHandle(processDataWindows->processStdIn.write), "closing stdin.write handle");
    FATAL_ERROR_IF_SYS_CALL_FAILED(CloseHandle(processDataWindows->synchronizationPipeParentToChild.write), "closing synchronizationPipeParentToChild.write handle");
    FATAL_ERROR_IF_SYS_CALL_FAILED(CloseHandle(processDataWindows->synchronizationPipeChildToParent.read), "closing synchronizationPipeChildToParent.read handle");
    FATAL_ERROR_IF_SYS_CALL_FAILED(CloseHandle(processDataWindows->measurementPipe.read), "closing measurementPipe.read handle");

    delete processDataWindows;
    this->osSpecificData = nullptr;
}

void Process::waitForFinish() {
    ProcessDataWindows *processDataWindows = static_cast<ProcessDataWindows *>(this->osSpecificData);
    if (processDataWindows->ended) {
        return;
    }

    if (WaitForSingleObject(processDataWindows->processInfo.hProcess, std::numeric_limits<DWORD>::max()) != WAIT_OBJECT_0) {
        FATAL_ERROR(std::string("waiting for process to end, ") + getErrorFromLastErrorCode());
    }

    if (processDataWindows->asyncReadThread->joinable()) {
        processDataWindows->asyncReadThread->join();
    }

    processDataWindows->ended = true;
}

TestResult Process::getResult() {
    ProcessDataWindows *processDataWindows = static_cast<ProcessDataWindows *>(this->osSpecificData);
    if (!processDataWindows->hasResult) {
        waitForFinish();

        DWORD exitCode{};
        FATAL_ERROR_IF_SYS_CALL_FAILED(GetExitCodeProcess(processDataWindows->processInfo.hProcess, &exitCode), "retrieving process exit code");

        processDataWindows->hasResult = true;
        processDataWindows->result = static_cast<TestResult>(exitCode);
    }
    return processDataWindows->result;
}

const std::string &Process::getStdout() {
    ProcessDataWindows *processDataWindows = static_cast<ProcessDataWindows *>(this->osSpecificData);
    waitForFinish();
    return processDataWindows->stdOut;
}

const std::string &Process::getMeasurements() {
    ProcessDataWindows *processDataWindows = static_cast<ProcessDataWindows *>(this->osSpecificData);

    if (!processDataWindows->hasMeasurements) {
        waitForFinish();
        processDataWindows->measurements = readEntirePipe(processDataWindows->measurementPipe.read);
        processDataWindows->hasMeasurements = true;
    }

    return processDataWindows->measurements;
}

void Process::synchronizationSignal() {
    ProcessDataWindows *processDataWindows = static_cast<ProcessDataWindows *>(this->osSpecificData);

    char buffer = ProcessSynchronizationHelper::synchronizationChar;
    DWORD numberOfBytesWritten = {};
    FATAL_ERROR_IF_SYS_CALL_FAILED(WriteFile(processDataWindows->synchronizationPipeParentToChild.write, &buffer, 1, &numberOfBytesWritten, nullptr), "writing to child process's synchronization pipe");
    FATAL_ERROR_IF(numberOfBytesWritten == 0, "No character was written when signalling child process");
}

void Process::synchronizationWait() {
    ProcessDataWindows *processDataWindows = static_cast<ProcessDataWindows *>(this->osSpecificData);

    char buffer = {};
    DWORD numberOfBytesRead = {};
    FATAL_ERROR_IF_SYS_CALL_FAILED(ReadFile(processDataWindows->synchronizationPipeChildToParent.read, &buffer, 1, &numberOfBytesRead, NULL), "reading from child process's synchronization pipe");
    FATAL_ERROR_IF(numberOfBytesRead == 0, "No character was read when waiting on a child process");
    FATAL_ERROR_IF(buffer != ProcessSynchronizationHelper::synchronizationChar, std::string("Invalid synchronization received from child process: '") + buffer + "'");
}
