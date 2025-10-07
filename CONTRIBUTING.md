<!---
Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT
-->

# Contribution guidelines
This document is intended for *non-Intel* contributors only.
If you are an Intel Employee and you want to contribute to Compute Benchmarks, please contact one of the [maintainers](https://github.com/intel/compute-benchmarks/blob/master/.config).

This document provides a basic set of instructions and rules for contributing to the Compute Benchmarks repository.
It is structured into two sections:
- Contribution process overview - describing pull request (referred to later as PR) submission
- Contributing to Compute Benchmarks - outlining processes specific to the Compute Benchmarks project

## Table of Contents

- [1. Contribution process overview](#contribution-overview)
  - [1.1 Commit message](#commit-message)
  - [1.2 Certificate of origin](#certificate)
  - [1.3 PR submission](#pr-submission)
  - [1.4 Initial review](#initial-review)
  - [1.5 Verification](#verification)
  - [1.6 Code review](#code-review)
  - [1.7 PR disposition](#pr-disposition)
- [2. Contributing to Compute Benchmarks](#benchmarks-contributing)
  - [2.1 Adding new benchmarks](#adding-new-benchmark)
  - [2.2 Generating documentation](#benchmarks-docs)
  - [2.3 SPIR-V translation](#spirv-translation)

## 1. Contribution process overview <a id="contribution-overview"></a>
### 1.1 Commit message <a id="commit-message"></a>

To make project history more readable a specific structure for the commit message is required:

```
Commit title
<BLANK LINE>
Commit body
<BLANK LINE>
Signed-off-by: Example Author example.author@example.com
```

Where:
* `Commit title` - should concisely describe the work introduced by the PR. The maximum title length is 50 characters.
* `Commit body` - should provide justification of the work and, optionally, additional details. The maximum body line length is 80 characters.
* `Signed-off-by` - should be put at the end of the commit message. See below.

### 1.2 Certificate of origin <a id="certificate"></a>

To establish a clear contribution chain of trust
[signed-off-by language](https://developercertificate.org/) is used.
Ensure that your commit message adheres to this guideline by containing the `Signed-off-by:` phrase.

The signature can be added to the commit message by passing short-option `-s` to `git commit`:
```
git commit -s
```

### 1.3 PR submission <a id="pr-submission"></a>

Before submitting a PR:
1. Use `clang-format` to properly format the code. You can use:
    * [Visual Studio extension](https://marketplace.visualstudio.com/items?itemName=LLVMExtensions.ClangFormat)
    * [Visual Studio code extension](https://marketplace.visualstudio.com/items?itemName=xaver.clang-format)
    * Script for formatting all files - [Windows](scripts/run_clang-format_on_all_files.cmd)/[Linux](scripts/run_clang-format_on_all_files.sh)
2. Make sure benchmarks build successfully with your change added. For details about building please read
[building](https://github.com/intel/compute-benchmarks/blob/master/README.md#building).
3. Remember to add a copyright header at the top of newly created files.
4. Remember to update the date in the copyright headers of the files you are updating.

Once all of the above is done, PR can be submitted.

### 1.4 Initial review <a id="initial-review"></a>

One of Compute Benchmarks maintainers will do an initial review of your code.
We will let you know if anything major is missing.

### 1.5 Verification <a id="verification"></a>

We'll double-check that your code meets our minimal quality expectations. Every commit must:
* Build successfully under Linux - using recent clang and gcc
* Build successfully under Windows
* Pass clang-format (using project configuration)

Once the automated verification succeeds, we will start the actual code review.

### 1.6 Code review <a id="code-review"></a>

We'll make sure that your code fits within the architecture and design of Compute Benchmarks, is readable
and maintainable. Please make sure to address our questions and concerns.

### 1.7 PR disposition <a id="pr-disposition"></a>

We reserve, upon conclusion of the code review, the right to do one of the following:
1. Merge the PR as submitted.
2. Merge the PR (with modifications).
3. Reject the PR.

If merged, you will be listed as the commit author.
Your commit may be reverted if a major regression is identified post-merge.

## 2. Contributing to Compute Benchmarks <a id="benchmarks-contributing"></a>

### 2.1 Adding new benchmarks <a id="adding-new-benchmark"></a>
A good way to add new benchmarks is to mimic the existing ones and tweak them to your needs. The general process for adding a brand new test is as follows:
1. Select a binary that suits your benchmark, for example `memory_benchmark`.
2. Choose a name for your benchmark, for example `TwoWayTransfer`.
3. Add a definition file of your benchmark as `source/benchmarks/memory_benchmark/definitions/TwoWayTransfer.h`. This file specifies general information about your test, including its name, description and parameters.
4. Add a test registration file as `source/benchmarks/memory_benchmark/gtest/TwoWayTransfer.cpp`. This file registers your test, so the framework recognizes it and can execute it.
5. Add an implementation file as `source/benchmarks/memory_benchmark/implementations/ocl/TwoWayTransfer_ocl.cpp`. This file contains the actual implementation of your test. Replace *ocl* with *l0* for LevelZero implementation. Each test *can* be implemented in more than one API.
6. Regenerate documentation (see below).

### 2.2 Generating documentation <a id="benchmarks-docs"></a>
Test documentation is generated from the code and stored in the [TESTS.md](TESTS.md) file. Contributors are required to regenerate the documentation by building the `run_docs_generator` target. [TESTS.md](TESTS.md) should be generated with *only* OpenCL and Level Zero enabled - otherwise, the generated file may contain incorrect contents. No further parameters are needed. After generating, include `TESTS.md` as part of the commit.

### 2.3 SPIRV translation <a id="spirv-translation"></a>
OpenCL kernel files (\*.cl) can be translated into SPIR-V representation files (\*.spv) using `ocloc` (OpenCL Offline Compiler) tool developed and maintained in [compute-runtime](https://github.com/intel/compute-runtime/tree/master/shared/offline_compiler) repository. Compute Benchmarks provide [compile_to_spv.sh](https://github.com/intel/compute-benchmarks/blob/master/scripts/compile_to_spv.sh) utility script to help with the procedure. Script requires the `ocloc` binary to be present in your PATH. The `ocloc` binary can be acquired from [compute-runtime releases](https://github.com/intel/compute-runtime/releases/) (using the [latest](https://github.com/intel/compute-runtime/releases/latest) release is strongly recommended).

*Note: At the time of writing this guide, latest release is `25.35.35096.9` and such version will be used in the following examples.*

To generate SPIR-V files using the script, follow these steps:
1. Download and install the `ocloc` package.
```
wget https://github.com/intel/compute-runtime/releases/download/25.35.35096.9/intel-ocloc_25.35.35096.9-0_amd64.deb
dpkg -i intel-ocloc_25.35.35096.9-0_amd64.deb
```
2. Download and install the `igc-core` package to satisfy the `libigdfcl.so.2` dependency for `ocloc`:
```
wget https://github.com/intel/intel-graphics-compiler/releases/download/v2.18.5/intel-igc-core-2_2.18.5+19820_amd64.deb
dpkg -i intel-igc-core-2_2.18.5+19820_amd64.deb
```
3. Add `/usr/local/lib` to the `LD_LIBRARY_PATH`:
```
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
```
4. Run the script (replace with your kernel path):
```
./scripts/compile_to_spv.sh /compute-benchmarks/source/benchmarks/record_and_replay_benchmark/kernels/graph_api_benchmark_kernel_assign.cl
```
5. The generated .spv file will be written to your current directory.
