# Contributing to ComputeBenchmarks
Please use ClangFormat to properly format the code. 
  - [Visual Studio extension](https://marketplace.visualstudio.com/items?itemName=LLVMExtensions.ClangFormat).
  - [Visual Studio code extension](https://marketplace.visualstudio.com/items?itemName=xaver.clang-format)
  - Script formatting all files [Windows](scripts/run_clang-format_on_all_files.cmd) [Linux](scripts/run_clang-format_on_all_files.sh)

Submit your changes via pull request. One of the maintainers will review your changes and merge them.

Remember to add a copyright header at the top of newly created files.

### Adding new benchmarks
A good way to add new benchmarks is to mimic the existing ones and tweak them to your needs. General flow of adding a brand new test is:
1. Select binary, that suits you benchmark, for example `memory_benchmark`
2. Select name for you benchmark, for example `TwoWayTransfer`.
3. Add definition file of your benchmark as source/benchmarks/`memory_benchmark`/definitions/`TwoWayTransfer`.h. This file specifies generic info about your test - its name, description and parameters
4. Add test registration file as source/benchmarks/`memory_benchmark`/gtest/`TwoWayTransfer`.cpp. This file registers your test, so the framework knows about it and it can be run.
5. Add implementation file as source/benchmarks/`memory_benchmark`/implementations/ocl/`TwoWayTransfer`_ocl.cpp. This file is contains the actual implementation of your test. Substitute *ocl* with *l0* for LevelZero implementation. Each test *can* be implemented in more than one API.
6. Regenerate documentation (see below).

### Generating documentation
Tests documentation is generated from the code and stored in [TESTS.md](TESTS.md) file. Contributors are required to regenerate the documentation by building the *run_docs_generator* target. No further parameters are needed. The [TESTS.md](TESTS.md) file should be automatically updated.
