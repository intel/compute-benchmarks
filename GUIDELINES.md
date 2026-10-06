<!---
Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT
-->

# Coding guidelines
This document lists the coding rules for Compute Benchmarks. They apply to all new and modified code. `clang-format` checks the formatting automatically; the code review checks the rules below.

## Table of Contents

- [1. C++ usage](#cpp-usage)
- [2. Naming conventions](#naming-conventions)
- [3. Coding rules](#coding-rules)
- [4. Error handling](#error-handling)
- [5. Benchmark implementation](#benchmark-implementation)
- [6. Comments](#comments)
- [7. Documentation](#documentation)

## 1. C++ usage <a id="cpp-usage"></a>
* Use C++ style casts instead of C style casts.
* Do not use default parameters.
* Prefer `using` over `typedef`.
* Use `constexpr` instead of `#define` for constants.
* Do not define global constants with internal linkage in header files; use C++17 `inline` variables instead.
* Do not use `using namespace` in headers.
* Manage ownership with smart pointers (`std::unique_ptr`, `std::make_unique`); do not use raw `new` and `delete`.
* Prefer const-correctness: mark members, locals and parameters `const` where possible, and pass non-trivial types by const reference.
* Prefer named constants over magic numbers. The parameter values in a gtest registration file are the test configuration and stay inline, for example `::testing::Values(128 * megaByte)`.
* Inside methods, use an explicit `this->` pointer to refer to non-static class members.
* Do not use Unicode characters in code.

## 2. Naming conventions <a id="naming-conventions"></a>
* Use snake_case for file names, for example `usm_copy.h`, `usm_copy.cpp` and `usm_copy_l0.cpp`.
* Use PascalCase for class, struct, enum and namespace names, and for the test case name returned by `getTestCaseName()`.
* Use camelCase for variable and function names, and for the command line keys of test case arguments.
* Prefer verbose names for variables and functions:
```
bad examples : sz, cnt, buf
good examples: bufferSize, kernelCount, stagingBuffer
```

## 3. Coding rules <a id="coding-rules"></a>
* Favor self-explanatory code over comments, see [Comments](#comments).
* Avoid code duplication. When two benchmarks need the same logic, put it in a helper in `source/framework` and reuse the existing helpers there, instead of copying code between benchmarks.
* Refactor existing code when it improves clarity or removes duplication introduced by your change.
* One-line branches use braces.
* Headers are guarded using `#pragma once`.
* Do not use `TODO`s in the code.
* Do not leave dead or commented-out code. This includes code that is disabled with a `#define` switch set to `0`.

## 4. Error handling <a id="error-handling"></a>
* Do not use `assert`. It is removed in Release builds, so the check does not run in the configuration that is measured.
* Check every API call with the macro of that API, for example `ASSERT_ZE_RESULT_SUCCESS` or `ASSERT_CL_SUCCESS`. On failure, the macro prints the error and returns `TestResult::Error`, so only the current test case fails.
* When the device, the driver or the API cannot run a configuration, return the `TestResult` that names the reason, for example `DeviceNotCapable`, `ApiNotCapable` or `DriverFunctionNotFound`. Do not report it as `TestResult::Error`.
* Use `FATAL_ERROR` and `FATAL_ERROR_IF` only when the framework is in a state from which it cannot continue, for example an incorrect use of a framework class. They stop the whole binary, not only the current test case.
* Do not throw exceptions in benchmark code; return a `TestResult` instead.

## 5. Benchmark implementation <a id="benchmark-implementation"></a>
* Measure only what is required. The timed region between `timer.measureStart()` and `timer.measureEnd()` contains the operation under test and nothing else. Collateral work, for example the creation of resources that the operation uses, data initialization and result verification, stays outside of it.
* Handle `isNoopRun()` before you create resources: push the unit and type of the measurement to `statistics` and return `TestResult::Nooped`.
* Keep the test case independent of other test cases; do not rely on the execution order or on state that a different test case leaves behind.

## 6. Comments <a id="comments"></a>
The expected number of comments added by a change is zero; every comment has to earn its place on its own. That the file being modified already contains comments is not a reason to add another one.

* A comment earns its place when it carries a fact that lives outside the source code and that a future edit would break silently, for example: a hardware, specification or driver behavior that the measurement depends on (name the document or the workaround identifier), an ordering, a warmup or an extra synchronization that reads as arbitrary but is required for the measured number to be valid, a constant whose origin cannot be derived from the code, a language or toolchain constraint that forces the shape of the code, or the reason why the obvious simpler form is wrong.
* Do not restate what the code already says, i.e. `// synchronize the queue` above a `finish()` call.
* Do not document why the change was made or what the bug was; that is the content of the commit message.
* Do not pre-empt an expected review objection in the code; answer it in the pull request instead.
* Do not add a comment describing a test case in its definition, registration or implementation file. The test case name and its arguments identify the scenario, and the user-facing description of what is measured belongs in `getHelp()` and in the argument help strings, which is what [TESTS.md](TESTS.md) is generated from.
* When a comment is only needed because the code is hard to follow, fix the code instead: a named `constexpr`, a better function or variable name, or a helper whose name is the explanation. Those survive refactoring, a comment does not.
* In C++ sources, use double slash instead of block comments, except for the copyright header, which stays in its required block form.

## 7. Documentation <a id="documentation"></a>
* Keep documentation in sync with the code, and update it in the same change. When a change adds, removes or changes a test case, an argument or a help string, regenerate [TESTS.md](TESTS.md). When a change modifies the build, run or contribution flow that [README.md](README.md), [FAQ.md](FAQ.md) or [CONTRIBUTING.md](CONTRIBUTING.md) describes, update that document.
* Do not leave stale documentation behind.
* Keep code, comments, help strings and documentation generic and free of internal-only or non-public information, because they may be published.
